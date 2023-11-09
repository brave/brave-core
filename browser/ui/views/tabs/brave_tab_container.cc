/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_container.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/check_is_test.h"
#include "base/containers/flat_map.h"
#include "brave/browser/ui/tabs/brave_tab_layout_constants.h"
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
#include "chrome/grit/theme_resources.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/display/screen.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/skbitmap_operations.h"
#include "ui/views/view_utils.h"

namespace {

gfx::Size AddHorizontalTabStripSpacing(gfx::Size size) {
  if (!tabs::features::HorizontalTabsUpdateEnabled()) {
    return size;
  }
  // Allow for a small space at the top and bottom of the tab strip. Tab group
  // underlines will partially occupy the space below tabs.
  size.Enlarge(0, brave_tabs::kHorizontalTabStripVerticalSpacing * 2);
  return size;
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
      tab_style_(TabStyle::Get()),
      controller_(controller) {
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

gfx::Size BraveTabContainer::GetMinimumSize() const {
  gfx::Size size = TabContainerImpl::GetMinimumSize();
  if (tabs::utils::ShouldShowVerticalTabs(tab_slot_controller_->GetBrowser())) {
    return size;
  }
  return AddHorizontalTabStripSpacing(size);
}

gfx::Size BraveTabContainer::CalculatePreferredSize() const {
  // Note that we check this before checking currently we're in vertical tab
  // strip mode. We might be in the middle of changing orientation.
  if (layout_locked_) {
    return {};
  }

  if (!tabs::utils::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    return AddHorizontalTabStripSpacing(
        TabContainerImpl::CalculatePreferredSize());
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
  if (!tabs::utils::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    return TabContainerImpl::GetDropIndex(event);
  }

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

      // Closing tabs should be skipped.
      if (tab->closing()) {
        continue;
      }

      const int model_index = GetModelIndexOf(tab).value();

      const bool is_tab_pinned = tab->data().pinned;

      // When dropping text or links onto pinned tabs, we need to take the
      // x-axis position into consideration.
      if (is_tab_pinned && x >= max_x) {
        continue;
      }

      const bool first_in_group =
          tab->group().has_value() &&
          model_index == controller_->GetFirstTabInGroup(tab->group().value());

      const int hot_height = tab->height() / 4;
      const int hot_width = tab->width() / 4;

      if (is_tab_pinned ? x >= (max_x - hot_width)
                        : y >= (max_y - hot_height)) {
        return {model_index + 1, true /* drop_before */,
                false /* drop_in_group */};
      }

      if (is_tab_pinned ? x < tab->x() + hot_width
                        : y < tab->y() + hot_height) {
        return {model_index, true /* drop_before */, first_in_group};
      }

      return {model_index, false /* drop_before */, false /* drop_in_group */};
    } else {
      TabGroupHeader* const group_header = static_cast<TabGroupHeader*>(view);
      const int first_tab_index =
          controller_->GetFirstTabInGroup(group_header->group().value())
              .value();
      return {first_tab_index, true /* drop_before */,
              y >= max_y - group_header->height() / 2 /* drop_in_group */};
    }
  }

  // The drop isn't over a tab, add it to the end.
  return {GetTabCount(), true, false};
}

// BraveTabContainer::DropArrow:
// ----------------------------------------------------------
BraveTabContainer::DropArrow::DropArrow(const BrowserRootView::DropIndex& index,
                                        Position position,
                                        bool beneath,
                                        views::Widget* context)
    : index_(index), position_(position), beneath_(beneath) {
  arrow_window_ = new views::Widget;
  views::Widget::InitParams params(views::Widget::InitParams::TYPE_POPUP);
  params.z_order = ui::ZOrderLevel::kFloatingUIElement;
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  params.accept_events = false;

  // All drop images has the same size.
  const gfx::ImageSkia* drop_image =
      GetDropArrowImage(Position::Horizontal, false);
  params.bounds = gfx::Rect(drop_image->width(), drop_image->height());

  params.context = context->GetNativeWindow();
  arrow_window_->Init(std::move(params));
  arrow_view_ =
      arrow_window_->SetContentsView(std::make_unique<views::ImageView>());
  arrow_view_->SetImage(GetDropArrowImage(position_, beneath_));
  scoped_observation_.Observe(arrow_window_.get());

  arrow_window_->Show();
}

BraveTabContainer::DropArrow::~DropArrow() {
  // Close eventually deletes the window, which deletes arrow_view too.
  if (arrow_window_) {
    arrow_window_->Close();
  }
}

void BraveTabContainer::DropArrow::SetBeneath(bool beneath) {
  if (beneath_ == beneath) {
    return;
  }

  beneath_ = beneath;
  arrow_view_->SetImage(GetDropArrowImage(position_, beneath));
}

void BraveTabContainer::DropArrow::SetWindowBounds(const gfx::Rect& bounds) {
  arrow_window_->SetBounds(bounds);
}

void BraveTabContainer::DropArrow::OnWidgetDestroying(views::Widget* widget) {
  DCHECK(scoped_observation_.IsObservingSource(arrow_window_.get()));
  scoped_observation_.Reset();
  arrow_window_ = nullptr;
}

void BraveTabContainer::HandleDragUpdate(
    const absl::optional<BrowserRootView::DropIndex>& index) {
  if (!tabs::utils::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    TabContainerImpl::HandleDragUpdate(index);
    return;
  }
  SetDropArrow(index);
}

void BraveTabContainer::HandleDragExited() {
  if (!tabs::utils::ShouldShowVerticalTabs(
          tab_slot_controller_->GetBrowser())) {
    TabContainerImpl::HandleDragExited();
    return;
  }
  SetDropArrow({});
}

gfx::Rect BraveTabContainer::GetDropBounds(int drop_index,
                                           bool drop_before,
                                           bool drop_in_group,
                                           bool* is_beneath) {
  DCHECK_NE(drop_index, -1);

  // The center is determined along the x-axis if it's pinned, or along the
  // y-axis if not.
  int center = -1;

  if (GetTabCount() == 0) {
    // If the tabstrip is empty, it doesn't matter where the drop arrow goes.
    // The tabstrip can only be transiently empty, e.g. during shutdown.
    return gfx::Rect();
  }

  Tab* tab = GetTabAtModelIndex(std::min(drop_index, GetTabCount() - 1));

  const bool is_tab_pinned = tab->data().pinned;

  const bool first_in_group =
      drop_index < GetTabCount() && tab->group().has_value() &&
      GetModelIndexOf(tab) ==
          controller_->GetFirstTabInGroup(tab->group().value());

  if (!drop_before || !first_in_group || drop_in_group) {
    // Dropping between tabs, or between a group header and the group's first
    // tab.
    center = is_tab_pinned ? tab->x() : tab->y();
    const int length = is_tab_pinned ? tab->width() : tab->height();
    if (drop_index < GetTabCount()) {
      center += drop_before ? -(tabs::kVerticalTabsSpacing / 2) : (length / 2);
    } else {
      center += length + (tabs::kVerticalTabsSpacing / 2);
    }
  } else {
    // Dropping before a group header.
    TabGroupHeader* const header = group_views_[tab->group().value()]->header();
    // Since there is no tab group in pinned tabs, there is no need to consider
    // the x-axis.
    center = header->y() + tabs::kVerticalTabsSpacing / 2;
  }

  // Since all drop indicator images are the same size, we will use the right
  // arrow image to determine the height and width.
  const gfx::ImageSkia* drop_image = GetDropArrowImage(
      BraveTabContainer::DropArrow::Position::Horizontal, false);

  // Determine the screen bounds.
  gfx::Point drop_loc(is_tab_pinned ? center - drop_image->width() / 2 : 0,
                      is_tab_pinned ? tab->y() - drop_image->height()
                                    : center - drop_image->height() / 2);
  ConvertPointToScreen(this, &drop_loc);
  gfx::Rect drop_bounds(drop_loc.x(), drop_loc.y(), drop_image->width(),
                        drop_image->height());

  // If the rect doesn't fit on the monitor, push the arrow to the bottom.
  display::Screen* screen = display::Screen::GetScreen();
  display::Display display = screen->GetDisplayMatching(drop_bounds);
  *is_beneath = !display.bounds().Contains(drop_bounds);

  if (*is_beneath) {
    drop_bounds.Offset(
        is_tab_pinned ? 0 : drop_bounds.width() + tab->width(),
        is_tab_pinned ? drop_bounds.height() + tab->height() : 0);
  }

  return drop_bounds;
}

gfx::ImageSkia* BraveTabContainer::GetDropArrowImage(
    BraveTabContainer::DropArrow::Position pos,
    bool beneath) {
  using Position = BraveTabContainer::DropArrow::Position;
  using RotationAmount = SkBitmapOperations::RotationAmount;
  static base::NoDestructor<
      base::flat_map<std::pair<Position, bool>, gfx::ImageSkia>>
      drop_images([] {
        gfx::ImageSkia* top_arrow_image =
            ui::ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
                IDR_TAB_DROP_UP);

        base::flat_map<std::pair<Position, bool>, gfx::ImageSkia>
            position_to_images;

        position_to_images.emplace(std::make_pair(Position::Vertical, true),
                                   *top_arrow_image);
        position_to_images.emplace(
            std::make_pair(Position::Horizontal, false),
            gfx::ImageSkiaOperations::CreateRotatedImage(
                *top_arrow_image, RotationAmount::ROTATION_90_CW));
        position_to_images.emplace(
            std::make_pair(Position::Vertical, false),
            gfx::ImageSkiaOperations::CreateRotatedImage(
                *top_arrow_image, RotationAmount::ROTATION_180_CW));
        position_to_images.emplace(
            std::make_pair(Position::Horizontal, true),
            gfx::ImageSkiaOperations::CreateRotatedImage(
                *top_arrow_image, RotationAmount::ROTATION_270_CW));
        return position_to_images;
      }());
  return &drop_images->find(std::make_pair(pos, beneath))->second;
}

void BraveTabContainer::SetDropArrow(
    const absl::optional<BrowserRootView::DropIndex>& index) {
  if (!index) {
    controller_->OnDropIndexUpdate(absl::nullopt, false);
    drop_arrow_.reset();
    return;
  }

  // Let the controller know of the index update.
  controller_->OnDropIndexUpdate(index->value, index->drop_before);

  if (drop_arrow_ && (index == drop_arrow_->index())) {
    return;
  }

  bool is_beneath = false;
  gfx::Rect drop_bounds = GetDropBounds(index->value, index->drop_before,
                                        index->drop_in_group, &is_beneath);

  if (!drop_arrow_) {
    DropArrow::Position position = DropArrow::Position::Vertical;
    if (GetTabCount() > 0) {
      Tab* tab = GetTabAtModelIndex(0);
      position = tab->data().pinned ? DropArrow::Position::Vertical
                                    : DropArrow::Position::Horizontal;
    }
    drop_arrow_ =
        std::make_unique<DropArrow>(*index, position, is_beneath, GetWidget());
  } else {
    drop_arrow_->set_index(*index);
    drop_arrow_->SetBeneath(is_beneath);
  }

  // Reposition the window.
  drop_arrow_->SetWindowBounds(drop_bounds);
}

BEGIN_METADATA(BraveTabContainer, TabContainerImpl)
END_METADATA
