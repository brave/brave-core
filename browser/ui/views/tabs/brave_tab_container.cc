/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_container.h"

#include <algorithm>
#include <vector>

#include "base/check_is_test.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/tabs/tab_drag_controller.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/canvas.h"
#include "ui/views/view_utils.h"

namespace {
VerticalTabStripRegionView::State GetVerticalTabState(const Browser* browser) {
  DCHECK(browser);
  auto* browser_view = static_cast<BraveBrowserView*>(
      BrowserView::GetBrowserViewForBrowser(browser));
  if (browser_view) {
    auto* vertical_region_view =
        browser_view->vertical_tab_strip_widget_delegate_view()
            ->vertical_tab_strip_region_view();
    DCHECK(vertical_region_view);

    return vertical_region_view->state();
  }
  return VerticalTabStripRegionView::State();
}
}  // namespace

BraveTabContainer::BraveTabContainer(
    TabContainerController& controller,
    TabHoverCardController* hover_card_controller,
    TabDragContextBase* drag_context,
    TabSlotController& tab_slot_controller,
    views::View* scroll_contents_view)
    : TabContainerImpl(controller,
                       hover_card_controller,
                       drag_context,
                       tab_slot_controller,
                       scroll_contents_view),
      drag_context_(static_cast<TabDragContext*>(drag_context)),
      tab_style_(TabStyle::Get()) {
  auto* browser = tab_slot_controller_->GetBrowser();
  if (!browser) {
    CHECK_IS_TEST();
    return;
  }

  if (!tabs::utils::SupportsVerticalTabs(browser)) {
    return;
  }

  auto* prefs = browser->profile()->GetOriginalProfile()->GetPrefs();
  show_vertical_tabs_.Init(
      brave_tabs::kVerticalTabsEnabled, prefs,
      base::BindRepeating(&BraveTabContainer::UpdateLayoutOrientation,
                          base::Unretained(this)));
  vertical_tabs_floating_mode_enabled_.Init(
      brave_tabs::kVerticalTabsFloatingEnabled, prefs,
      base::BindRepeating(&BraveTabContainer::UpdateLayoutOrientation,
                          base::Unretained(this)));
  vertical_tabs_collapsed_.Init(
      brave_tabs::kVerticalTabsCollapsed, prefs,
      base::BindRepeating(&BraveTabContainer::UpdateLayoutOrientation,
                          base::Unretained(this)));

  UpdateLayoutOrientation();
}

BraveTabContainer::~BraveTabContainer() {
  // When the last tab is closed and tab strip is being destructed, the
  // animation for the last removed tab could have been scheduled but not
  // finished yet. In this case, stop the animation before checking if all
  // closed tabs were cleaned up from OnTabCloseAnimationCompleted().
  CancelAnimation();
  DCHECK(closing_tabs_.empty()) << "There are dangling closed tabs.";
  DCHECK(!layout_locked_)
      << "The lock returned by LockLayout() shouldn't outlive this object";
}

base::OnceClosure BraveTabContainer::LockLayout() {
  DCHECK(!layout_locked_) << "LockLayout() doesn't allow reentrance";
  layout_locked_ = true;
  return base::BindOnce(&BraveTabContainer::OnUnlockLayout,
                        base::Unretained(this));
}

gfx::Size BraveTabContainer::CalculatePreferredSize() const {
  // Note that we check this before checking currently we're in vertical tab
  // strip mode. We might be in the middle of changing orientation.
  if (layout_locked_) {
    return {};
  }

  if (!tabs::utils::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    return TabContainerImpl::CalculatePreferredSize();
  }

  const int tab_count = tabs_view_model_.view_size();
  int height = 0;
  if (bounds_animator_.IsAnimating() && tab_count &&
      !drag_context_->GetDragController()->IsActive()) {
    // When removing a tab in the middle of tabs, the last tab's current bottom
    // could be greater than ideal bounds bottom.
    height = tabs_view_model_.view_at(tab_count - 1)->bounds().bottom();
  }

  if (!closing_tabs_.empty()) {
    // When closing trailing tabs, the last tab's current bottom could be
    // greater than ideal bounds bottom. Note that closing tabs are not in
    // tabs_view_model_ so we have to check again here.
    for (auto* tab : closing_tabs_) {
      height = std::max(height, tab->bounds().bottom());
    }
  }

  const auto slots_bounds = layout_helper_->CalculateIdealBounds(
      available_width_callback_.is_null() ||
              base::FeatureList::IsEnabled(features::kScrollableTabStrip)
          ? absl::nullopt
          : absl::optional<int>(available_width_callback_.Run()));
  height =
      std::max(height, slots_bounds.empty() ? 0 : slots_bounds.back().bottom());

  if (tab_count) {
    if (Tab* last_tab = tabs_view_model_.view_at(tab_count - 1);
        last_tab->group().has_value() &&
        !controller_->IsGroupCollapsed(*last_tab->group())) {
      height += BraveTabGroupHeader::kPaddingForGroup;
    }

    // Both containers for pinned tabs and unpinned tabs should have margin
    height += tabs::kMarginForVerticalTabContainers;
  }

  return gfx::Size(tab_style_->GetStandardWidth(), height);
}

void BraveTabContainer::UpdateClosingModeOnRemovedTab(int model_index,
                                                      bool was_active) {
  // Don't shrink vertical tab strip's width
  if (tabs::utils::ShouldShowVerticalTabs(tab_slot_controller_->GetBrowser())) {
    return;
  }

  TabContainerImpl::UpdateClosingModeOnRemovedTab(model_index, was_active);
}

gfx::Rect BraveTabContainer::GetTargetBoundsForClosingTab(
    Tab* tab,
    int former_model_index) const {
  if (!tabs::utils::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    return TabContainerImpl::GetTargetBoundsForClosingTab(tab,
                                                          former_model_index);
  }

  gfx::Rect target_bounds = tab->bounds();
  if (tab->data().pinned) {
    target_bounds.set_width(0);
  } else {
    target_bounds.set_y(
        (former_model_index > 0)
            ? tabs_view_model_.ideal_bounds(former_model_index - 1).bottom()
            : 0);
    target_bounds.set_height(0);
  }
  return target_bounds;
}

void BraveTabContainer::EnterTabClosingMode(absl::optional<int> override_width,
                                            CloseTabSource source) {
  // Don't shrink vertical tab strip's width
  if (tabs::utils::ShouldShowVerticalTabs(tab_slot_controller_->GetBrowser())) {
    return;
  }

  TabContainerImpl::EnterTabClosingMode(override_width, source);
}

bool BraveTabContainer::ShouldTabBeVisible(const Tab* tab) const {
  // We don't have to clip tabs out of bounds. Scroll view will handle it.
  if (tabs::utils::ShouldShowVerticalTabs(tab_slot_controller_->GetBrowser())) {
    return true;
  }

  return TabContainerImpl::ShouldTabBeVisible(tab);
}

void BraveTabContainer::StartInsertTabAnimation(int model_index) {
  // Note that we check this before checking currently we're in vertical tab
  // strip mode. We might be in the middle of changing orientation.
  if (layout_locked_) {
    return;
  }

  if (!tabs::utils::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    TabContainerImpl::StartInsertTabAnimation(model_index);
    return;
  }

  ExitTabClosingMode();

  auto* new_tab = GetTabAtModelIndex(model_index);
  gfx::Rect bounds = new_tab->bounds();
  bounds.set_height(tabs::kVerticalTabHeight);
  const auto tab_width = new_tab->data().pinned
                             ? tabs::kVerticalTabMinWidth
                             : tab_style_->GetStandardWidth();
  bounds.set_width(tab_width);
  bounds.set_x(-tab_width);
  bounds.set_y((model_index > 0)
                   ? tabs_view_model_.ideal_bounds(model_index - 1).bottom()
                   : 0);
  GetTabAtModelIndex(model_index)->SetBoundsRect(bounds);

  // Animate in to the full width.
  AnimateToIdealBounds();
}

void BraveTabContainer::RemoveTab(int index, bool was_active) {
  if (tabs::utils::ShouldShowVerticalTabs(tab_slot_controller_->GetBrowser())) {
    closing_tabs_.insert(tabs_view_model_.view_at(index));
  }

  TabContainerImpl::RemoveTab(index, was_active);
}

void BraveTabContainer::OnTabCloseAnimationCompleted(Tab* tab) {
  if (tabs::utils::ShouldShowVerticalTabs(tab_slot_controller_->GetBrowser())) {
    closing_tabs_.erase(tab);
  }

  TabContainerImpl::OnTabCloseAnimationCompleted(tab);

  // we might have to hide this container entirely
  if (!tabs_view_model_.view_size()) {
    PreferredSizeChanged();
  }
}

void BraveTabContainer::UpdateLayoutOrientation() {
  layout_helper_->set_use_vertical_tabs(
      tabs::utils::ShouldShowVerticalTabs(tab_slot_controller_->GetBrowser()));
  layout_helper_->set_tab_strip(
      static_cast<BraveTabStrip*>(base::to_address(tab_slot_controller_)));
  InvalidateLayout();
}

void BraveTabContainer::OnUnlockLayout() {
  layout_locked_ = false;

  InvalidateIdealBounds();
  PreferredSizeChanged();
  CompleteAnimationAndLayout();
}

void BraveTabContainer::CompleteAnimationAndLayout() {
  // Note that we check this before checking currently we're in vertical tab
  // strip mode. We might be in the middle of changing orientation.
  if (layout_locked_) {
    return;
  }

  TabContainerImpl::CompleteAnimationAndLayout();

  // Should force tabs to layout as they might not change bounds, which makes
  // insets not updated.
  base::ranges::for_each(children(), &views::View::Layout);
}

void BraveTabContainer::OnPaintBackground(gfx::Canvas* canvas) {
  if (!tabs::utils::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    TabContainerImpl::OnPaintBackground(canvas);
    return;
  }

  canvas->DrawColor(GetColorProvider()->GetColor(kColorToolbar));
}

void BraveTabContainer::PaintChildren(const views::PaintInfo& paint_info) {
  // Exclude tabs that own layer.
  std::vector<ZOrderableTabContainerElement> orderable_children;
  for (views::View* child : children()) {
    if (!ZOrderableTabContainerElement::CanOrderView(child)) {
      continue;
    }
    if (child->layer()) {
      continue;
    }

    orderable_children.emplace_back(child);
  }

  std::stable_sort(orderable_children.begin(), orderable_children.end());

  for (const ZOrderableTabContainerElement& child : orderable_children) {
    child.view()->Paint(paint_info);
  }
}

BrowserRootView::DropIndex BraveTabContainer::GetDropIndex(
    const ui::DropTargetEvent& event) {
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs) ||
      !tabs::utils::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    return TabContainerImpl::GetDropIndex(event);
  }

  bool is_vtab_expanded =
      GetVerticalTabState(tab_slot_controller_->GetBrowser()) ==
      VerticalTabStripRegionView::State::kExpanded;

  // Force animations to stop, otherwise it makes the index calculation tricky.
  CompleteAnimationAndLayout();

  const int x = GetMirroredXInView(event.x());
  const int y = event.y();

  std::vector<TabSlotView*> views = layout_helper_->GetTabSlotViews();

  // Loop until we find a tab or group header that intersects |event|'s
  // location.
  for (TabSlotView* view : views) {
    const int max_y = view->y() + view->height();
    const int max_x = view->x() + view->width();
    if (y >= max_y) {
      continue;
    }

    if (view->GetTabSlotViewType() == TabSlotView::ViewType::kTab) {
      Tab* const tab = static_cast<Tab*>(view);
      // When the vertical tab strip state is expanded, pinned tabs are
      // organized in a grid view. Therefore, it's important to also consider
      // the x-axis location.
      if (is_vtab_expanded && tab->data().pinned && x >= max_x) {
        continue;
      }

      // Closing tabs should be skipped.
      if (tab->closing()) {
        continue;
      }

      const int model_index = GetModelIndexOf(tab).value();
      const bool first_in_group =
          tab->group().has_value() &&
          model_index == controller_->GetFirstTabInGroup(tab->group().value());

      // When hovering over the left or right quarter of a tab, the drop
      // indicator will point between tabs.
      const int hot_height = tab->height() / 4;

      if (y >= (max_y - hot_height)) {
        return {model_index + 1, true /* drop_before */,
                false /* drop_in_group */};
      } else if (y < tab->y() + hot_height) {
        return {model_index, true /* drop_before */, first_in_group};
      } else {
        // When the vertical tab is in an expanded state and also pinned, it's
        // important to confirm whether the event location is within the pinned
        // tab.
        if (is_vtab_expanded && tab->data().pinned &&
            !IsPointInTab(tab, gfx::Point(x, y))) {
          continue;
        }

        return {model_index, false /* drop_before */,
                false /* drop_in_group */};
      }
    } else {
      TabGroupHeader* const group_header = static_cast<TabGroupHeader*>(view);
      const int first_tab_index =
          controller_->GetFirstTabInGroup(group_header->group().value())
              .value();

      if (y < max_y - group_header->height() / 2) {
        return {first_tab_index, true /* drop_before */,
                false /* drop_in_group */};
      } else {
        return {first_tab_index, true /* drop_before */,
                true /* drop_in_group */};
      }
    }
  }

  // The drop isn't over a tab, add it to the end.
  return {GetTabCount(), true, false};
}

// TODO: Add functionality for 'HandleDragUpdate' and 'HandleDragExited'
// during the implementation of a drop arrow when dragging and dropping text
// or a link.
void BraveTabContainer::HandleDragUpdate(
    const absl::optional<BrowserRootView::DropIndex>& index) {
  if (base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs)) {
    return;
  }
  TabContainerImpl::HandleDragUpdate(index);
}

void BraveTabContainer::HandleDragExited() {
  if (base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs)) {
    return;
  }
  TabContainerImpl::HandleDragExited();
}

BEGIN_METADATA(BraveTabContainer, TabContainerImpl)
END_METADATA
