/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_container.h"

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/check_op.h"
#include "base/containers/flat_map.h"
#include "base/debug/stack_trace.h"
#include "base/feature_list.h"
#include "base/logging.h"
#include "base/notimplemented.h"
#include "base/strings/stringprintf.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/browser/ui/tabs/brave_compact_horizontal_tabs_layout.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/brave_tab.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_header.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_highlight.h"
#include "brave/browser/ui/views/tabs/brave_tab_group_underline.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip.h"
#include "brave/browser/ui/views/tabs/brave_tab_strip_layout_helper.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "brave/ui/color/nala/nala_color_id.h"
#include "cc/layers/layer.h"
#include "cc/paint/paint_flags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_style.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h"
#include "chrome/browser/ui/views/tabs/tab_container.h"
#include "chrome/browser/ui/views/tabs/tab_container_impl.h"
#include "chrome/browser/ui/views/tabs/tab_group_highlight.h"
#include "chrome/browser/ui/views/tabs/tab_strip_layout_helper.h"
#include "chrome/grit/theme_resources.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/split_tab_data.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/color/color_id.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/paint_recorder.h"
#include "ui/display/screen.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/geometry/transform.h"
#include "ui/gfx/image/image_skia_operations.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/gfx/skbitmap_operations.h"
#include "ui/views/background.h"
#include "ui/views/rect_based_targeting_utils.h"
#include "ui/views/style/platform_style.h"
#include "ui/views/view_utils.h"

using BrowserRootView::DropIndex::GroupInclusion::kDontIncludeInGroup;
using BrowserRootView::DropIndex::GroupInclusion::kIncludeInGroup;
using BrowserRootView::DropIndex::RelativeToIndex::kInsertBeforeIndex;
using BrowserRootView::DropIndex::RelativeToIndex::kReplaceIndex;

namespace {

// Calculates the scroll offset for horizontal tabs from both horizontal and
// vertical scroll events. Prioritizes horizontal scroll, falls back to
// vertical scroll.
template <class EventType>
int CalculateScrollOffset(const EventType& event,
                          views::LayoutOrientation scroll_direction) {
  if (scroll_direction == views::LayoutOrientation::kHorizontal &&
      event.x_offset() != 0) {
    return event.x_offset();
  }

  return event.y_offset();
}

void ReparentSlotViewLayerToParent(views::View* view, ui::Layer* parent_layer) {
  if (auto* tab = views::AsViewClass<BraveTab>(view)) {
    tab->ReparentLayerForUnpinnedScroll(parent_layer);
  } else if (auto* header = views::AsViewClass<BraveTabGroupHeader>(view)) {
    header->ReparentLayerForUnpinnedScroll(parent_layer);
  } else if (auto* underline =
                 views::AsViewClass<BraveTabGroupUnderline>(view)) {
    underline->ReparentLayerForUnpinnedScroll(parent_layer);
  } else if (auto* highlight =
                 views::AsViewClass<BraveTabGroupHighlight>(view)) {
    highlight->ReparentLayerForUnpinnedScroll(parent_layer);
  } else {
    DCHECK(!view->layer()) << view->GetClassName();
  }
}

void ReparentSlotViewLayerForScroll(views::View* view,
                                    ui::Layer* scroll_layer) {
  ReparentSlotViewLayerToParent(view, scroll_layer);
}

}  // namespace

BraveTabContainer::BraveTabContainer(
    TabContainerController& controller,
    TabHoverCardController* hover_card_controller,
    TabDragPositioningDelegateBase* drag_position_delegate,
    TabSlotController& tab_slot_controller)
    : TabContainerImpl(controller,
                       hover_card_controller,
                       drag_position_delegate,
                       tab_slot_controller),
      drag_context_(drag_position_delegate->GetContext()),
      tab_style_(TabStyle::Get()),
      controller_(controller) {
  auto* browser = tab_slot_controller_->GetBrowserWindowInterface();
  if (!browser) {
    CHECK_IS_TEST();
    return;
  }

  PrefService* prefs = browser->GetProfile()->GetPrefs();
  if (base::FeatureList::IsEnabled(tabs::kBraveScrollableTabStrip)) {
    scrollable_horizontal_tab_strip_.Init(
        brave_tabs::kScrollableHorizontalTabStrip, prefs,
        base::BindRepeating(
            &BraveTabContainer::OnScrollableHorizontalTabStripPrefChanged,
            base::Unretained(this)));
  }

  if (!tabs::utils::SupportsBraveVerticalTabs(browser)) {
    return;
  }

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
  if (base::FeatureList::IsEnabled(tabs::kBraveTreeTab)) {
    tree_tabs_enabled_.Init(
        brave_tabs::kTreeTabsEnabled, prefs,
        base::BindRepeating(&BraveTabContainer::OnTreeTabsEnabledChanged,
                            base::Unretained(this)));
  }
  should_show_scroll_bar_.Init(
      brave_tabs::kVerticalTabsShowScrollbar, prefs,
      base::BindRepeating(&BraveTabContainer::UpdateScrollBarVisibility,
                          base::Unretained(this)));

  // Create separator view between pinned and unpinned tabs
  separator_ = AddChildView(std::make_unique<views::View>());
  separator_->SetBackground(
      views::CreateSolidBackground(kColorBraveVerticalTabSeparator));

  // Create scroll bar
  scroll_bar_ = AddChildView(views::PlatformStyle::CreateScrollBar(
      views::ScrollBar::Orientation::kVertical));
  scroll_bar_->set_controller(this);

  UpdateLayoutOrientation();
  if (base::FeatureList::IsEnabled(tabs::kBraveTreeTab)) {
    OnTreeTabsEnabledChanged();
  }
}

BraveTabContainer::~BraveTabContainer() {
  TearDownScrollLayerStack();
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

bool BraveTabContainer::ShouldShowVerticalTabs() const {
  return tabs::utils::ShouldShowBraveVerticalTabs(
      tab_slot_controller_->GetBrowserWindowInterface());
}

views::ScrollView::ScrollBarMode BraveTabContainer::GetScrollBarMode() const {
  if (!ShouldShowVerticalTabs()) {
    return views::ScrollView::ScrollBarMode::kDisabled;
  }

  return *should_show_scroll_bar_
             ? views::ScrollView::ScrollBarMode::kEnabled
             : views::ScrollView::ScrollBarMode::kHiddenButEnabled;
}

gfx::Size BraveTabContainer::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  // Note that we check this before checking currently we're in vertical tab
  // strip mode. We might be in the middle of changing orientation.
  if (layout_locked_) {
    return {};
  }

  if (!ShouldShowVerticalTabs()) {
    return TabContainerImpl::CalculatePreferredSize(available_size);
  }

  const int tab_count = tabs_view_model_.view_size();
  int height = 0;
  if (bounds_animator_.IsAnimating() && tab_count &&
      !static_cast<TabDragContext*>(drag_context_.get())
           ->GetDragController()
           ->IsActive()) {
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

  const auto [slots_bounds, layout_domain] =
      layout_helper_->CalculateIdealBounds(
          available_width_callback_.is_null()
              ? std::nullopt
              : std::optional<int>(available_width_callback_.Run()));
  height =
      std::max(height, slots_bounds.empty() ? 0 : slots_bounds.back().bottom());

  if (tab_count) {
    height += tabs::kMarginForVerticalTabContainers;
  }

  // Passed |true| but it doesn't have any meaning becuase we always use same
  // width.
  return gfx::Size(tab_style_->GetStandardWidth(/*is_split*/ true), height);
}

void BraveTabContainer::UpdateClosingModeOnRemovedTab(int model_index,
                                                      bool was_active) {
  // Don't shrink vertical tab strip's width
  if (ShouldShowVerticalTabs()) {
    return;
  }

  TabContainerImpl::UpdateClosingModeOnRemovedTab(model_index, was_active);
}

gfx::Rect BraveTabContainer::GetTargetBoundsForClosingTab(
    Tab* tab,
    int former_model_index) const {
  if (!ShouldShowVerticalTabs()) {
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

void BraveTabContainer::EnterTabClosingMode(std::optional<int> override_width,
                                            CloseTabSource source) {
  // Don't shrink vertical tab strip's width
  if (ShouldShowVerticalTabs()) {
    return;
  }

  TabContainerImpl::EnterTabClosingMode(override_width, source);
}

bool BraveTabContainer::ShouldTabBeVisible(const Tab* tab) const {
  auto scroll_direction = GetScrollDirection();
  if (scroll_direction == views::LayoutOrientation::kVertical) {
    if (tab->dragging() || tab->detached()) {
      return true;
    }

    if (IsPinned(tab)) {
      return true;
    }

    if (auto tab_index = tabs_view_model_.GetIndexOfView(tab)) {
      const auto tab_bottom =
          tabs_view_model_.ideal_bounds(*tab_index).bottom();
      if (UsesLayerBasedScroll()) {
        return tab_bottom - scroll_offset_ > GetPinnedTabsAreaBottom();
      }
      return tab_bottom > GetPinnedTabsAreaBottom();
    }
  } else if (scroll_direction == views::LayoutOrientation::kHorizontal) {
    if (tab->data().pinned || tab->dragging()) {
      return TabContainerImpl::ShouldTabBeVisible(tab);
    }

    if (auto tab_index = tabs_view_model_.GetIndexOfView(tab)) {
      const int right = tabs_view_model_.ideal_bounds(*tab_index).right();
      if (UsesLayerBasedScroll()) {
        return right - scroll_offset_ > GetPinnedTabsAreaBoundary();
      }
      return right > GetPinnedTabsAreaBoundary();
    }
  }

  return TabContainerImpl::ShouldTabBeVisible(tab);
}

std::vector<Tab*> BraveTabContainer::AddTabs(
    std::vector<TabInsertionParams> tabs_params) {
  std::vector<Tab*> added_tabs =
      TabContainerImpl::AddTabs(std::move(tabs_params));
  if (GetScrollDirection()) {
    for (Tab* const tab : added_tabs) {
      ScrollTabToBeVisible(tab);
    }
  }
  return added_tabs;
}

void BraveTabContainer::StartInsertTabAnimation(int model_index) {
  // Note that we check this before checking currently we're in vertical tab
  // strip mode. We might be in the middle of changing orientation.
  if (layout_locked_) {
    return;
  }

  if (!ShouldShowVerticalTabs()) {
    TabContainerImpl::StartInsertTabAnimation(model_index);
    return;
  }

  ExitTabClosingMode();

  auto* new_tab = GetTabAtModelIndex(model_index);
  gfx::Rect bounds = new_tab->bounds();
  bounds.set_height(tabs::kVerticalTabHeight);
  const auto tab_width = IsPinned(new_tab) ? tabs::kVerticalTabMinWidth
                                           : tab_style_->GetStandardWidth(true);
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
  if (ShouldShowVerticalTabs()) {
    closing_tabs_.insert(tabs_view_model_.view_at(index));
  }

  TabContainerImpl::RemoveTab(index, was_active);
  ClampScrollOffset();
  UpdatePinnedUnpinnedSeparator();
}

void BraveTabContainer::OnTabCloseAnimationCompleted(Tab* tab) {
  if (ShouldShowVerticalTabs()) {
    closing_tabs_.erase(tab);
  }

  TabContainerImpl::OnTabCloseAnimationCompleted(tab);

  // we might have to hide this container entirely
  if (!tabs_view_model_.view_size()) {
    PreferredSizeChanged();
  }
}

void BraveTabContainer::UpdateLayoutOrientation() {
  bool enabled = ShouldShowVerticalTabs();
  layout_helper_->set_use_vertical_tabs(enabled);
  layout_helper_->set_tab_strip(
      static_cast<TabStrip*>(base::to_address(tab_slot_controller_)));

  // Tab could have different insets per orientation(ex, split tabs).
  const int tab_count = GetTabCount();
  for (int i = 0; i < tab_count; ++i) {
    GetTabAtModelIndex(i)->UpdateInsets();
  }

  if (enabled) {
    scroll_offset_ = 0;
  } else {
    // Clear any per-view clip paths; scrolling clips via layer stack or bounds.
    for (int i = 0; i < GetTabCount(); ++i) {
      GetTabAtModelIndex(i)->SetClipPath({});
    }
    for (auto& [_, group_views] : group_views_) {
      for (auto* view : std::initializer_list<views::View*>{
               group_views->header(), group_views->underline(),
               group_views->highlight(), group_views->drag_underline()}) {
        if (view) {
          view->SetClipPath({});
        }
      }
    }
  }

  // As the tab style varies per orientation, we should update all tabs' style.
  for (int i = 0; i < GetTabCount(); ++i) {
    views::AsViewClass<BraveTab>(GetTabAtModelIndex(i))->UpdateTabStyle();
  }

  if (!GetScrollDirection()) {
    TearDownScrollLayerStack();
  }

  UpdateScrollBarVisibility();

  InvalidateLayout();
}

void BraveTabContainer::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  if (!GetScrollDirection()) {
    TabContainerImpl::OnBoundsChanged(previous_bounds);
    return;
  }

  ClampScrollOffset();

  TabContainerImpl::OnBoundsChanged(previous_bounds);

  if (UsesLayerBasedScroll()) {
    UpdateScrollLayerBoundsAndClip();
    ApplyScrollLayerTransform();
  }
}

void BraveTabContainer::PaintBoundingBoxForSplitTabs(gfx::Canvas& canvas) {
  auto* tab_strip_model =
      tab_slot_controller_->GetBrowserWindowInterface()->GetTabStripModel();
  // Cache unique ids to avoid paiting same split tab twice.
  base::flat_set<split_tabs::SplitTabId> split_tab_ids;
  for (int i = 0; i < GetTabCount(); ++i) {
    Tab* tab = GetTabAtModelIndex(i);
    if (!tab->split().has_value()) {
      continue;
    }
    if (!tab_strip_model->ContainsSplit(*tab->split())) {
      // This can happen when detaching split tabs to a new window.
      continue;
    }
    auto tabs = tab_strip_model->GetSplitData(*tab->split())->ListTabs();
    if (tabs.empty()) {
      continue;
    }
    split_tab_ids.insert(*tab->split());
  }

  for (const auto& id : split_tab_ids) {
    auto tabs = tab_strip_model->GetSplitData(id)->ListTabs();
    CHECK(tabs.size() == 2);
    PaintBoundingBoxForSplitTab(canvas,
                                {tab_strip_model->GetIndexOfTab(tabs[0]),
                                 tab_strip_model->GetIndexOfTab(tabs[1])});
  }
}

void BraveTabContainer::PaintBoundingBoxForSplitTab(
    gfx::Canvas& canvas,
    const std::vector<int>& indices) {
  CHECK(indices.size() == 2);

  gfx::Rect bounding_rects;
  auto* tab1 = GetTabAtModelIndex(indices[0]);
  auto* tab2 = GetTabAtModelIndex(indices[1]);

  const bool is_vertical_tab = ShouldShowVerticalTabs();

  gfx::ScopedCanvas scoped_canvas(&canvas);
  if (is_vertical_tab && !tab1->data().pinned) {
    // We assume paired split tabs are both unpinned or both pinned.
    CHECK(!tab2->data().pinned);
    // clip canvas to avoid painting split tab bounding box in pinned tabs area
    const auto pinned_tabs_area_bottom = GetPinnedTabsAreaBottom();
    canvas.ClipRect(gfx::Rect(0, pinned_tabs_area_bottom, size().width(),
                              size().height() - pinned_tabs_area_bottom));
  }

  for (auto tab : {tab1, tab2}) {
    bounding_rects.Union(tab->bounds());
  }

  if (!is_vertical_tab) {
    // In order to make margin between the bounding box and tab strip.
    // Need to compensate the amount of overlap because it's hidden by overlap
    // at bottom.
    int vertical_margin = tab1->data().pinned ? 4 : 2;
    bounding_rects.Inset(gfx::Insets::TLBR(
        vertical_margin, tabs::kHorizontalTabInset,
        vertical_margin +
            GetLayoutConstant(LayoutConstant::kTabstripToolbarOverlap),
        tabs::kHorizontalTabInset));
  }

  const float kRadius =
      (!is_vertical_tab && tabs::ShouldUseCompactHorizontalTabsForNonTouchUI())
          ? tabs::compact_horizontal_tabs_layout::
                kHorizontalSplitViewTileCornerRadiusDip
          : 12.f;  // default matches --leo-radius-l

  auto* cp = GetColorProvider();
  DCHECK(cp);

  cc::PaintFlags flags;
  flags.setColor(cp->GetColor(
      is_vertical_tab ? kColorBraveSplitViewTileBackgroundVertical
                      : kColorBraveSplitViewTileBackgroundHorizontal));

  canvas.DrawRoundRect(bounding_rects, kRadius, flags);

  auto* tab_strip_model =
      tab_slot_controller_->GetBrowserWindowInterface()->GetTabStripModel();
  const auto active_tab_index = tab_strip_model->active_index();
  if (!is_vertical_tab && active_tab_index != indices[0] &&
      active_tab_index != indices[1] && !tab1->IsMouseHovered() &&
      !tab2->IsMouseHovered()) {
    constexpr int kSplitViewSeparatorHeight = 24;
    auto separator_top = bounding_rects.top_center();
    if (bounding_rects.height() <= kSplitViewSeparatorHeight) {
      // Bounding rect can be too small during shutdown.
      return;
    }
    // Calculate gap between tab bounds top and separator top.
    const int gap = (bounding_rects.height() - kSplitViewSeparatorHeight) / 2;
    separator_top.Offset(0, gap);
    auto separator_bottom = separator_top;
    separator_bottom.Offset(0, kSplitViewSeparatorHeight);
    canvas.DrawLine(separator_top, separator_bottom,
                    cp->GetColor(kColorBraveSplitViewTileDivider));
  }

  bounding_rects.Outset(1);
  flags.setStyle(cc::PaintFlags::kStroke_Style);
  flags.setColor(cp->GetColor(kColorBraveSplitViewTileBackgroundBorder));
  canvas.DrawRoundRect(bounding_rects, kRadius, flags);
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

  if (GetScrollDirection()) {
    last_layout_size_ = size();
  }

  TabContainerImpl::CompleteAnimationAndLayout();

  if (GetScrollDirection()) {
    EnsureScrollLayerStack();
    UpdateScrollLayerBoundsAndClip();
    ReparentUnpinnedSlotLayers();
    ApplyScrollLayerTransform();
  } else {
    TearDownScrollLayerStack();
  }

  UpdateClipPathForSlotViews();
  SetTabSlotVisibility();
  UpdatePinnedUnpinnedSeparator();
}

void BraveTabContainer::PaintChildren(const views::PaintInfo& paint_info) {
  // Custom z-order (see TabContainerImpl::PaintChildren). When layer-based
  // scroll is on, this view and unpinned slot views both own layers; calling
  // View::Paint on those children with our PaintInfo breaks DCHECK
  // (PaintContext::RootVisited) and is not how composited views record.
  // Paint their layers the same way the compositor does.
  std::vector<ZOrderableTabContainerElement> orderable_children;
  for (views::View* child : children()) {
    if (!ZOrderableTabContainerElement::CanOrderView(child)) {
      continue;
    }
    orderable_children.emplace_back(child);
  }

  std::stable_sort(orderable_children.begin(), orderable_children.end());

  auto* tab_strip =
      static_cast<TabStrip*>(base::to_address(tab_slot_controller_));
  if (!tab_strip->controller() || !tab_strip->GetBrowserWindowInterface()) {
    CHECK_IS_TEST();
    return;
  }

  {
    ui::PaintRecorder recorder(paint_info.context(),
                               paint_info.paint_recording_size(),
                               paint_info.paint_recording_scale_x(),
                               paint_info.paint_recording_scale_y(), nullptr);
    PaintBoundingBoxForSplitTabs(*recorder.canvas());
  }

  LOG(ERROR) << "[brave-tabs] PaintChildren self_layer=" << layer()
             << " self_layer_bounds="
             << (layer() ? layer()->bounds().ToString() : std::string("(null)"))
             << " bounds=" << bounds().ToString()
             << " orderable_count=" << orderable_children.size();

  for (const ZOrderableTabContainerElement& child : orderable_children) {
    // Layer-backed children record through their own ui::Layer (the compositor
    // calls Layer::PaintContentsToDisplayList), not through this paint walk.
    // Skipping them mirrors views::View::RecursivePaintHelper and avoids the
    // PaintContext::RootVisited DCHECK in View::Paint.
    LOG(ERROR) << "[brave-tabs]   child=" << child.view()
               << " class=" << child.view()->GetClassName()
               << " bounds=" << child.view()->bounds().ToString()
               << " layer=" << child.view()->layer()
               << " skip=" << (child.view()->layer() != nullptr);
    if (child.view()->layer()) {
      continue;
    }
    child.view()->Paint(paint_info);
  }

  if (!ShouldShowVerticalTabs()) {
    return;
  }

  // Paint the separator after all tabs so it appears on top
  if (separator_ && separator_->GetVisible()) {
    separator_->Paint(paint_info);
  }

  // Paint scroll bar
  if (!scroll_bar_->layer() && scroll_bar_->GetVisible()) {
    scroll_bar_->Paint(paint_info);
  }
}

void BraveTabContainer::SetTabSlotVisibility() {
  // During multiple tab closing including group, this method could be called
  // but group_views_ could be empty already. We should clear group info in tabs
  // in that case.
  // https://github.com/brave/brave-browser/issues/39298
  for (Tab* tab : layout_helper_->GetTabs()) {
    if (std::optional<tab_groups::TabGroupId> group = tab->group();
        group && !group_views_.contains(*group)) {
      tab->SetGroup(std::nullopt);
    }
  }

  TabContainerImpl::SetTabSlotVisibility();

  if (GetScrollDirection()) {
    // Even though TabContainerImpl::SetTabSlotVisibility() already updates the
    // bounds of the group views for certain cases, we need to update them again
    // https://github.com/brave/brave-browser/issues/51786#issuecomment-3716778522
    for (auto& [_, group_views] : group_views_) {
      group_views->UpdateBounds();
    }
  }
}

void BraveTabContainer::InvalidateIdealBounds() {
  if (GetScrollDirection()) {
    last_layout_size_ = std::nullopt;
  }

  if (IsAnimating()) {
    // In case of there's an animation in progress, we need to stop it first
    // before invalidating the ideal bounds. Otherwise, newly calculated ideal
    // bounds will be ignored.
    CancelAnimation();
  }

  TabContainerImpl::InvalidateIdealBounds();
  UpdatePinnedUnpinnedSeparator();
}

void BraveTabContainer::Layout(PassKey) {
  const auto scroll_direction = GetScrollDirection();
  if (!scroll_direction) {
    LayoutSuperclass<TabContainerImpl>(this);
    return;
  }

  // We don't need to layout if the height is 0 as it's invisible.
  if (scroll_direction == views::LayoutOrientation::kVertical && !height()) {
    return;
  }

  // If size hasn't changed since last layout, no need to relayout tabs.
  if (last_layout_size_ == size()) {
    return;
  }

  LayoutSuperclass<TabContainerImpl>(this);

  // After superclass layout, clear any stale per-view clip paths on slot views.
  SetTabSlotVisibility();
  UpdateClipPathForSlotViews();
  UpdatePinnedUnpinnedSeparator();

  UpdateScrollBarBounds();

  last_layout_size_ = size();

  if (GetScrollDirection()) {
    EnsureScrollLayerStack();
    UpdateScrollLayerBoundsAndClip();
    ReparentUnpinnedSlotLayers();
    ApplyScrollLayerTransform();
  }
}

void BraveTabContainer::ScrollTabToBeVisible(Tab* tab) {
  if (tab->data().pinned) {
    return;
  }

  auto scroll_direction = GetScrollDirection();
  if (!scroll_direction.has_value()) {
    return;
  }

  auto tab_index = tabs_view_model_.GetIndexOfView(tab);
  CHECK(tab_index);

  const gfx::Rect tab_ideal_bounds = tabs_view_model_.ideal_bounds(*tab_index);
  const int pinned_tabs_area_boundary = GetPinnedTabsAreaBoundary();

  if (UsesLayerBasedScroll()) {
    if (scroll_direction == views::LayoutOrientation::kVertical) {
      const int tab_top = tab_ideal_bounds.y();
      const int visible_content_top =
          pinned_tabs_area_boundary + scroll_offset_;

      if (tab_top < visible_content_top) {
        auto new_offset = scroll_offset_ - (visible_content_top - tab_top) -
                          tabs::kMarginForVerticalTabContainers;
        new_offset = std::clamp(new_offset, 0, GetMaxScrollOffset());
        SetScrollOffset(new_offset);
        return;
      }

      const int tab_bottom = tab_ideal_bounds.bottom();
      const int visible_content_bottom = height() + scroll_offset_;
      if (tab_bottom > visible_content_bottom) {
        auto new_offset = scroll_offset_ +
                          (tab_bottom - visible_content_bottom) +
                          tabs::kMarginForVerticalTabContainers;
        new_offset = std::clamp(new_offset, 0, GetMaxScrollOffset());
        SetScrollOffset(new_offset);
        return;
      }
    } else {
      const int tab_left = tab_ideal_bounds.x();
      const int visible_content_left =
          pinned_tabs_area_boundary + scroll_offset_;

      if (tab_left < visible_content_left) {
        auto new_offset = scroll_offset_ - (visible_content_left - tab_left);
        new_offset = std::clamp(new_offset, 0, GetMaxScrollOffset());
        SetScrollOffset(new_offset);
        return;
      }

      const int tab_right = tab_ideal_bounds.right();
      const int visible_content_right = width() + scroll_offset_;
      if (tab_right > visible_content_right) {
        auto new_offset = scroll_offset_ + (tab_right - visible_content_right);
        new_offset = std::clamp(new_offset, 0, GetMaxScrollOffset());
        SetScrollOffset(new_offset);
        return;
      }
    }
    return;
  }

  if (scroll_direction == views::LayoutOrientation::kVertical) {
    const int tab_top = tab_ideal_bounds.y();
    const int visible_area_top = pinned_tabs_area_boundary;

    if (tab_top < visible_area_top) {
      auto new_offset = scroll_offset_ - (visible_area_top - tab_top) -
                        tabs::kMarginForVerticalTabContainers;
      new_offset = std::clamp(new_offset, 0, GetMaxScrollOffset());
      SetScrollOffset(new_offset);
      return;
    }

    const int tab_bottom = tab_ideal_bounds.bottom();
    const int visible_area_bottom = height();
    if (tab_bottom > visible_area_bottom) {
      auto new_offset = scroll_offset_ + (tab_bottom - visible_area_bottom) +
                        tabs::kMarginForVerticalTabContainers;
      new_offset = std::clamp(new_offset, 0, GetMaxScrollOffset());
      SetScrollOffset(new_offset);
      return;
    }
  } else {
    const int tab_left = tab_ideal_bounds.x();
    const int visible_area_left = pinned_tabs_area_boundary;

    if (tab_left < visible_area_left) {
      auto new_offset = scroll_offset_ - (visible_area_left - tab_left);
      new_offset = std::clamp(new_offset, 0, GetMaxScrollOffset());
      SetScrollOffset(new_offset);
      return;
    }

    const int tab_right = tab_ideal_bounds.right();
    const int visible_area_right = width();
    if (tab_right > visible_area_right) {
      auto new_offset = scroll_offset_ + (tab_right - visible_area_right);
      new_offset = std::clamp(new_offset, 0, GetMaxScrollOffset());
      SetScrollOffset(new_offset);
      return;
    }
  }
}

void BraveTabContainer::OnScrollableHorizontalTabStripPrefChanged() {
  // only called when tabs::kBraveScrollableTabStrip feature flag is enabled.

  if (!IsHorizontalScrollableTabStripEnabled()) {
    SetScrollOffset(0);
  }
  InvalidateIdealBounds();
  InvalidateLayout();
}

void BraveTabContainer::UpdateScrollBarVisibility() {
  if (!scroll_bar_) {
    return;
  }

  const auto scroll_bar_mode = GetScrollBarMode();
  const bool should_show_scroll_bar =
      scroll_bar_mode == views::ScrollView::ScrollBarMode::kEnabled &&
      GetUnpinnedTabsViewportHeight() < GetUnpinnedTabsTotalHeight();
  if (should_show_scroll_bar == scroll_bar_->GetVisible()) {
    return;
  }

  scroll_bar_->SetVisible(should_show_scroll_bar);

  // "Ignored" removes the scrollbar from the accessibility tree.
  // "IsLeaf" removes their children (e.g. the buttons and thumb).
  const bool is_scroll_bar_disabled =
      scroll_bar_mode == views::ScrollView::ScrollBarMode::kDisabled;
  scroll_bar_->GetViewAccessibility().SetIsIgnored(is_scroll_bar_disabled);
  scroll_bar_->GetViewAccessibility().SetIsLeaf(is_scroll_bar_disabled);

  UpdateScrollBarBounds();
}

void BraveTabContainer::UpdateScrollBarBounds() {
  if (!scroll_bar_ || !scroll_bar_->GetVisible()) {
    return;
  }

  CHECK_EQ(*GetScrollDirection(), views::LayoutOrientation::kVertical);

  const int pinned_tabs_area_boundary = GetPinnedTabsAreaBoundary();

  // Vertical scrollbar on the right side
  scroll_bar_->SetBounds(width() - scroll_bar_->GetThickness(),
                         pinned_tabs_area_boundary, scroll_bar_->GetThickness(),
                         height() - pinned_tabs_area_boundary);

  UpdateScrollBarState();

  if (scroll_bar_->GetVisible()) {
    scroll_bar_->SchedulePaint();
  }
}

void BraveTabContainer::UpdateScrollBarState() {
  if (!scroll_bar_) {
    return;
  }

  if (scroll_bar_->GetVisible()) {
    scroll_bar_->Update(GetUnpinnedTabsViewportHeight(),
                        GetUnpinnedTabsTotalHeight(), scroll_offset_);
    GetViewAccessibility().SetScrollYMin(scroll_bar_->GetMinPosition());
    GetViewAccessibility().SetScrollYMax(scroll_bar_->GetMaxPosition());
  } else {
    scroll_bar_->Update(0, 0, 0);
    GetViewAccessibility().SetScrollYMin(0);
    GetViewAccessibility().SetScrollYMax(0);
  }
}

bool BraveTabContainer::HandleScroll(int offset) {
  if (offset == 0) {
    return false;
  }

  const auto scroll_direction = GetScrollDirection();
  if (!scroll_direction.has_value()) {
    return false;
  }

  // Apply the scroll offset
  int new_offset = scroll_offset_ - offset;
  new_offset = std::clamp(new_offset, 0, GetMaxScrollOffset());
  SetScrollOffset(new_offset);
  return true;
}

std::optional<views::LayoutOrientation> BraveTabContainer::GetScrollDirection()
    const {
  if (ShouldShowVerticalTabs()) {
    return views::LayoutOrientation::kVertical;
  }

  if (IsHorizontalScrollableTabStripEnabled()) {
    return views::LayoutOrientation::kHorizontal;
  }

  return std::nullopt;
}

void BraveTabContainer::OnSplitCreated(const std::vector<int>& indices) {
  UpdateTabsBorderInSplitTab(indices);
}

void BraveTabContainer::OnSplitRemoved(const std::vector<int>& indices) {
  UpdateTabsBorderInSplitTab(indices);
}

void BraveTabContainer::OnSplitContentsChanged(
    const std::vector<int>& indices) {
  UpdateTabsBorderInSplitTab(indices);
}

bool BraveTabContainer::OnMouseWheel(const ui::MouseWheelEvent& event) {
  const auto scroll_direction = GetScrollDirection();
  if (!scroll_direction) {
    return TabContainerImpl::OnMouseWheel(event);
  }

  if (HandleScroll(CalculateScrollOffset(event, *scroll_direction))) {
    return true;
  }

  return TabContainerImpl::OnMouseWheel(event);
}

void BraveTabContainer::OnScrollEvent(ui::ScrollEvent* event) {
  const auto scroll_direction = GetScrollDirection();
  if (!scroll_direction.has_value()) {
    TabContainerImpl::OnScrollEvent(event);
    return;
  }

  if (HandleScroll(CalculateScrollOffset(*event, *scroll_direction))) {
    event->SetHandled();
    return;
  }

  TabContainerImpl::OnScrollEvent(event);
}

views::View* BraveTabContainer::GetTooltipHandlerForPoint(
    const gfx::Point& point) {
  if (!UsesLayerBasedScroll()) {
    return TabContainerImpl::GetTooltipHandlerForPoint(point);
  }

  if (!GetVisible()) {
    return nullptr;
  }

  if (!HitTestPoint(point)) {
    return nullptr;
  }

  views::View* v = views::View::GetTooltipHandlerForPoint(point);
  if (v && v != this && !views::IsViewClass<Tab>(v)) {
    return v;
  }

  views::View* tab = FindTabHitByPoint(AdjustPointForSlotViewHitTest(point));
  if (tab) {
    return tab;
  }

  return this;
}

views::View* BraveTabContainer::TargetForRect(views::View* root,
                                              const gfx::Rect& rect) {
  const auto scroll_direction = GetScrollDirection();
  if (!scroll_direction) {
    return TabContainerImpl::TargetForRect(root, rect);
  }

  if (scroll_bar_ && scroll_bar_->GetVisible() &&
      scroll_bar_->bounds().Intersects(rect)) {
    gfx::Rect rect_in_scroll_bar_coords =
        views::View::ConvertRectToTarget(this, scroll_bar_, rect);
    return scroll_bar_->GetEventHandlerForRect(rect_in_scroll_bar_coords);
  }

  if (!UsesLayerBasedScroll()) {
    return TabContainerImpl::TargetForRect(root, rect);
  }

  if (!views::UsePointBasedTargeting(rect)) {
    return views::ViewTargeterDelegate::TargetForRect(root, rect);
  }

  const gfx::Point point = AdjustPointForSlotViewHitTest(rect.CenterPoint());

  views::View* v = views::ViewTargeterDelegate::TargetForRect(root, rect);
  if (v && v != this && !views::IsViewClass<Tab>(v)) {
    return v;
  }

  views::View* tab = FindTabHitByPoint(point);
  if (tab) {
    return tab;
  }

  return this;
}

bool BraveTabContainer::IsPointInTab(
    Tab* tab,
    const gfx::Point& point_in_tabstrip_coords) {
  const gfx::Point p =
      (!IsPinned(tab) && UsesLayerBasedScroll())
          ? AdjustPointForSlotViewHitTest(point_in_tabstrip_coords)
          : point_in_tabstrip_coords;
  return TabContainerImpl::IsPointInTab(tab, p);
}

void BraveTabContainer::AnimateToIdealBounds() {
  if (GetScrollDirection() == views::LayoutOrientation::kVertical) {
    // Pre-compute ideal bounds so we can snap expanding tabs before the base
    // method sets up animations. When a collapsed group is expanded, its tabs
    // go from height 0 to full height. Without this, BoundsAnimator animates
    // that transition, making favicons appear to grow. By snapping first,
    // AnimateTabSlotViewTo() sees they're already at target and skips them,
    // while tabs below still animate smoothly into position.
    UpdateIdealBounds();

    for (int i = 0; i < GetTabCount(); ++i) {
      Tab* tab = GetTabAtModelIndex(i);
      if (tab->bounds().height() == 0 && tab->group().has_value() &&
          !controller_->IsGroupCollapsed(*tab->group())) {
        tab->SetBoundsRect(tabs_view_model_.ideal_bounds(i));
      }
    }
  }

  TabContainerImpl::AnimateToIdealBounds();

  if (!GetScrollDirection()) {
    return;
  }

  // We need to update the visibility of the tab slot views after the animation
  // is kicked off. TabContainerImpl::AnimateToIdealBounds() calls
  // UpdateIdealBounds() but it doesn't call SetTabSlotVisibility().
  // https://github.com/brave/brave-browser/issues/51781
  SetTabSlotVisibility();
}

void BraveTabContainer::UpdateIdealBounds() {
  if (GetScrollDirection() && GetWidget()) {
    EnsureScrollLayerStack();
  }

  TabContainerImpl::UpdateIdealBounds();

  auto scroll_direction = GetScrollDirection();
  if (!scroll_direction) {
    return;
  }

  if (!UsesLayerBasedScroll()) {
    // Without a compositor layer stack, keep scroll baked into ideal bounds.
    int tab_count = GetTabCount();
    for (int i = 0; i < tab_count; ++i) {
      Tab* tab = GetTabAtModelIndex(i);
      CHECK(tab);
      if (tab->data().pinned) {
        continue;
      }

      gfx::Rect bounds = tabs_view_model_.ideal_bounds(i);
      if (*scroll_direction == views::LayoutOrientation::kVertical) {
        bounds.set_y(bounds.y() - scroll_offset_);
      } else {
        bounds.set_x(bounds.x() - scroll_offset_);
      }
      tabs_view_model_.set_ideal_bounds(i, bounds);
    }

    for (auto& [group_id, _] : group_views_) {
      auto& bounds = layout_helper_->group_header_ideal_bounds().at(group_id);
      if (*scroll_direction == views::LayoutOrientation::kVertical) {
        bounds.set_y(bounds.y() - scroll_offset_);
      } else {
        bounds.set_x(bounds.x() - scroll_offset_);
      }
    }
  }

  // Keep scroll offset in range without re-entering layout (ClampScrollOffset
  // may call CompleteAnimationAndLayout while ideal bounds are updating).
  const int clamped = std::clamp(scroll_offset_, 0, GetMaxScrollOffset());
  if (clamped != scroll_offset_) {
    scroll_offset_ = clamped;
  }

  UpdateScrollBarVisibility();
  UpdateScrollBarBounds();

  if (UsesLayerBasedScroll()) {
    UpdateScrollLayerBoundsAndClip();
    ReparentUnpinnedSlotLayers();
    ApplyScrollLayerTransform();
  }
}

void BraveTabContainer::OnTabSlotAnimationProgressed(TabSlotView* view) {
  TabContainerImpl::OnTabSlotAnimationProgressed(view);

  if (!GetScrollDirection()) {
    return;
  }

  if (Tab* tab = views::AsViewClass<Tab>(view)) {
    if (tab->data().pinned) {
      return;
    }
  }

  if (!bounds_animator_.IsAnimating()) {
    // Do not try clamp scroll offset when there's ongoing animation.
    // This could cause infinite loop.
    // More specifically, when ShouldRenderRichAnimation() returns false,
    // then we could be in the middle of canceling animation, and checking if
    // animation queue is empty in a while loop - see BoundsAnimator::Cancel. As
    // ClampScrollOffset() could queue another animation for slot views, we
    // should not try clamp scroll offset.
    // https://github.com/brave/brave-browser/issues/52044
    ClampScrollOffset();
  }
}

void BraveTabContainer::SetActiveTab(std::optional<size_t> prev_active_index,
                                     std::optional<size_t> new_active_index) {
  TabContainerImpl::SetActiveTab(prev_active_index, new_active_index);
  // Scroll to make the active tab visible for both vertical and horizontal
  // scrollable tab strip. ScrollTabToBeVisible no-ops when not scrollable.
  if (GetScrollDirection() && new_active_index.has_value()) {
    ScrollTabToBeVisible(GetTabAtModelIndex(*new_active_index));
  }
}

std::optional<BrowserRootView::DropIndex> BraveTabContainer::GetDropIndex(
    const ui::DropTargetEvent& event) {
  if (!ShouldShowVerticalTabs()) {
    return TabContainerImpl::GetDropIndex(event);
  }

  // Force animations to stop, otherwise it makes the index calculation
  // tricky.
  CompleteAnimationAndLayout();

  const int x = GetMirroredXInView(event.x());
  const gfx::Point adjusted_point =
      UsesLayerBasedScroll()
          ? AdjustPointForSlotViewHitTest(gfx::Point(x, event.y()))
          : gfx::Point(x, event.y());
  const int y = adjusted_point.y();

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

      const bool is_tab_pinned = tab->data().pinned;

      // When dropping text or links onto pinned tabs, we need to take the
      // x-axis position into consideration.
      if (is_tab_pinned && x >= max_x) {
        continue;
      }

      const int model_index = GetModelIndexOf(tab).value();
      const bool first_in_group =
          tab->group().has_value() &&
          model_index == controller_->GetFirstTabInGroup(tab->group().value());

      const int hot_height = tab->height() / 4;
      const int hot_width = tab->width() / 4;

      if (is_tab_pinned ? x >= (max_x - hot_width)
                        : y >= (max_y - hot_height)) {
        return BrowserRootView::DropIndex{
            .index = model_index + 1,
            .relative_to_index = kInsertBeforeIndex,
            .group_inclusion = kDontIncludeInGroup};
      }

      if (is_tab_pinned ? x < tab->x() + hot_width
                        : y < tab->y() + hot_height) {
        return BrowserRootView::DropIndex{
            .index = model_index,
            .relative_to_index = kInsertBeforeIndex,
            .group_inclusion =
                first_in_group ? kIncludeInGroup : kDontIncludeInGroup};
      }

      return BrowserRootView::DropIndex{.index = model_index,
                                        .relative_to_index = kReplaceIndex,
                                        .group_inclusion = kIncludeInGroup};
    } else {
      TabGroupHeader* const group_header = static_cast<TabGroupHeader*>(view);
      const int first_tab_index =
          controller_->GetFirstTabInGroup(group_header->group().value())
              .value();
      return BrowserRootView::DropIndex{
          .index = first_tab_index,
          .relative_to_index = kInsertBeforeIndex,
          .group_inclusion = y >= max_y - group_header->height() / 2
                                 ? kIncludeInGroup
                                 : kDontIncludeInGroup};
    }
  }

  // The drop isn't over a tab, add it to the end.
  return BrowserRootView::DropIndex{.index = GetTabCount(),
                                    .relative_to_index = kInsertBeforeIndex,
                                    .group_inclusion = kDontIncludeInGroup};
}

// BraveTabContainer::DropArrow:
// ----------------------------------------------------------
BraveTabContainer::DropArrow::DropArrow(const BrowserRootView::DropIndex& index,
                                        Position position,
                                        bool beneath,
                                        views::Widget* context)
    : index_(index), position_(position), beneath_(beneath) {
  arrow_window_ = std::make_unique<views::Widget>();
  views::Widget::InitParams params(
      views::Widget::InitParams::CLIENT_OWNS_WIDGET,
      views::Widget::InitParams::TYPE_POPUP);
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
  arrow_view_->SetImage(
      ui::ImageModel::FromImageSkia(*GetDropArrowImage(position_, beneath_)));

  arrow_window_->Show();
}

BraveTabContainer::DropArrow::~DropArrow() = default;

void BraveTabContainer::DropArrow::SetBeneath(bool beneath) {
  if (beneath_ == beneath) {
    return;
  }

  beneath_ = beneath;
  arrow_view_->SetImage(
      ui::ImageModel::FromImageSkia(*GetDropArrowImage(position_, beneath)));
}

void BraveTabContainer::DropArrow::SetWindowBounds(const gfx::Rect& bounds) {
  arrow_window_->SetBounds(bounds);
}

void BraveTabContainer::HandleDragUpdate(
    const std::optional<BrowserRootView::DropIndex>& index) {
  if (!ShouldShowVerticalTabs()) {
    TabContainerImpl::HandleDragUpdate(index);
    return;
  }
  SetDropArrow(index);
}

void BraveTabContainer::HandleDragExited() {
  if (!ShouldShowVerticalTabs()) {
    TabContainerImpl::HandleDragExited();
    return;
  }
  SetDropArrow({});
}

void BraveTabContainer::ScrollToPosition(views::ScrollBar* source,
                                         int position) {
  SetScrollOffset(std::clamp(position, 0, GetMaxScrollOffset()));
}

int BraveTabContainer::GetScrollIncrement(views::ScrollBar* source,
                                          bool is_page,
                                          bool is_positive) {
  return tabs::kVerticalTabHeight;
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
    // Since there is no tab group in pinned tabs, there is no need to
    // consider the x-axis.
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
  display::Screen* screen = display::Screen::Get();
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
    const std::optional<BrowserRootView::DropIndex>& index) {
  if (!index) {
    controller_->OnDropIndexUpdate(std::nullopt, false);
    drop_arrow_.reset();
    return;
  }

  // Let the controller know of the index update.
  controller_->OnDropIndexUpdate(
      index->index,
      index->relative_to_index ==
          BrowserRootView::DropIndex::RelativeToIndex::kInsertBeforeIndex);

  if (drop_arrow_ && (index == drop_arrow_->index())) {
    return;
  }

  bool is_beneath = false;
  gfx::Rect drop_bounds = GetDropBounds(
      index->index,
      index->relative_to_index ==
          BrowserRootView::DropIndex::RelativeToIndex::kInsertBeforeIndex,
      index->group_inclusion ==
          BrowserRootView::DropIndex::GroupInclusion::kIncludeInGroup,
      &is_beneath);

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

void BraveTabContainer::UpdateTabsBorderInSplitTab(
    const std::vector<int>& indices) {
  for (auto& index : indices) {
    if (!controller_->IsValidModelIndex(index)) {
      // In case the tiled tab is not in this container, this can happen.
      // For instance, this container is for pinned tabs but tabs in the tile
      // are unpinned.
      return;
    }

    // Tab's border varies per split view state.
    // See BraveVerticalTabStyle::GetContentsInsets().
    GetTabAtModelIndex(index)->UpdateInsets();
  }

  AnimateToIdealBounds();
}

int BraveTabContainer::GetPinnedTabsAreaBottom() const {
  const int pinned_tab_count = layout_helper_->GetPinnedTabCount();
  if (pinned_tab_count == 0) {
    return 0;
  }

  // Note that we should use ideal bounds instead of current bounds of the
  // last pinned tab because the pinned tab could be being dragged over
  // unpinned tab area.
  int bottom = GetIdealBounds(pinned_tab_count - 1).bottom() +
               tabs::kMarginForVerticalTabContainers;  // spacing after the last
                                                       // pinned tab

  // Add separator height if we have both pinned and unpinned tabs
  if (pinned_tab_count < GetTabCount()) {
    // Note that we don't add spacing after the separator as unpinned tabs will
    // be laid out with spacing regardless of the separator.
    bottom += tabs::kPinnedUnpinnedSeparatorHeight;
  }

  return bottom;
}

int BraveTabContainer::GetPinnedTabsAreaBoundary() const {
  auto scroll_direction = GetScrollDirection();
  if (!scroll_direction) {
    return GetPinnedTabsAreaBottom();
  }
  if (scroll_direction == views::LayoutOrientation::kVertical) {
    return GetPinnedTabsAreaBottom();
  }

  // For horizontal tabs, calculate the right boundary
  const int pinned_tab_count = layout_helper_->GetPinnedTabCount();
  if (pinned_tab_count == 0) {
    return 0;
  }

  // Note that we should use ideal bounds instead of current bounds of the
  // last pinned tab because the pinned tab could be being dragged over
  // unpinned tab area.
  return GetIdealBounds(pinned_tab_count - 1).right();
}

void BraveTabContainer::SetScrollOffset(int offset) {
  const auto scroll_direction = GetScrollDirection();
  if (!scroll_direction) {
    return;
  }

  const int clamped = std::clamp(offset, 0, GetMaxScrollOffset());
  if (clamped == scroll_offset_) {
    return;
  }

  scroll_offset_ = clamped;

  if (UsesLayerBasedScroll()) {
    ApplyScrollLayerTransform();
    UpdateScrollBarState();
    if (*scroll_direction == views::LayoutOrientation::kHorizontal) {
      horizontal_scroll_offset_changed_callbacks_.Notify();
    }
    return;
  }

  // First layout or tests: fall back to full layout until the layer stack
  // exists.
  last_layout_size_ = std::nullopt;
  CompleteAnimationAndLayout();
  UpdateScrollBarState();

  if (*scroll_direction == views::LayoutOrientation::kHorizontal) {
    horizontal_scroll_offset_changed_callbacks_.Notify();
  }
}

base::CallbackListSubscription
BraveTabContainer::RegisterHorizontalScrollOffsetChangedCallback(
    base::RepeatingClosure callback) {
  return horizontal_scroll_offset_changed_callbacks_.Add(std::move(callback));
}

std::pair<TabSlotView*, TabSlotView*>
BraveTabContainer::FindVisibleUnpinnedSlotViews() const {
  auto is_slot_visible = [this](TabSlotView* view) {
    if (view->GetTabSlotViewType() == TabSlotView::ViewType::kTab) {
      Tab* tab = static_cast<Tab*>(view);
      // Skip pinned tabs
      if (tab->data().pinned) {
        return false;
      }

      // Skip closing tabs. Closing tabs are visible but should not be
      // considered when calculating the max scroll offset.
      if (tab->closing()) {
        return false;
      }

      // Tabs in collapsed group are not visible
      if (auto group = tab->group()) {
        return !controller_->IsGroupCollapsed(*group);
      }

      return true;
    }

    // Group header is always visible
    return true;
  };

  std::vector<TabSlotView*> slot_views = layout_helper_->GetTabSlotViews();
  TabSlotView* first_visible = nullptr;
  TabSlotView* last_visible = nullptr;

  // Find first visible unpinned slot view
  // is_slot_visible already checks if the slot is unpinned
  for (TabSlotView* view : slot_views) {
    if (is_slot_visible(view)) {
      first_visible = view;
      break;
    }
  }

  // Find last visible unpinned slot view
  // is_slot_visible already checks if the slot is unpinned
  for (auto it = slot_views.rbegin(); it != slot_views.rend(); ++it) {
    TabSlotView* view = *it;
    if (is_slot_visible(view)) {
      last_visible = view;
      break;
    }
  }

  return std::make_pair(first_visible, last_visible);
}

gfx::Rect BraveTabContainer::GetIdealBoundsOf(TabSlotView* slot_view) const {
  if (slot_view->GetTabSlotViewType() == TabSlotView::ViewType::kTab) {
    Tab* tab = static_cast<Tab*>(slot_view);
    std::optional<int> model_index = GetModelIndexOf(tab);
    if (model_index.has_value()) {
      return tabs_view_model_.ideal_bounds(model_index.value());
    }
  } else {
    // Group header
    TabGroupHeader* group_header = static_cast<TabGroupHeader*>(slot_view);
    if (group_header->group().has_value()) {
      return layout_helper_->group_header_ideal_bounds().at(
          group_header->group().value());
    }
  }
  return gfx::Rect();
}

int BraveTabContainer::GetUnpinnedTabsTotalHeight() const {
  const int pinned_tab_count = layout_helper_->GetPinnedTabCount();
  const int tab_count = GetTabCount();
  if (tab_count == pinned_tab_count) {
    // No pinned tabs, so the total height is 0
    return 0;
  }

  auto [first_slot_view, last_slot_view] = FindVisibleUnpinnedSlotViews();
  if (!first_slot_view || !last_slot_view) {
    // All tabs may already have data().pinned == true (set eagerly via
    // TabModel::UpdateProperties during MoveTabsRecursive) while the layout
    // helper still has a stale count. This can happen mid-notification when
    // pinning a split - both tabs get UpdateProperties at once, but
    // layout_helper_ is updated one notification at a time.
    return 0;
  }
  int total_height = 0;

  const gfx::Rect first_bounds = GetIdealBoundsOf(first_slot_view);
  const gfx::Rect last_bounds = GetIdealBoundsOf(last_slot_view);

  total_height += std::max(0, last_bounds.bottom() - first_bounds.y());

  // Add margins
  total_height += 2 * tabs::kMarginForVerticalTabContainers;

  return total_height;
}

int BraveTabContainer::GetUnpinnedTabsViewportHeight() const {
  return height() - GetPinnedTabsAreaBottom();
}

int BraveTabContainer::GetUnpinnedTabsTotalSize() const {
  auto scroll_direction = GetScrollDirection();
  if (!scroll_direction) {
    return 0;
  }

  if (scroll_direction == views::LayoutOrientation::kVertical) {
    return GetUnpinnedTabsTotalHeight();
  }

  // For horizontal tabs, calculate total width from visible unpinned slots
  auto [first_slot_view, last_slot_view] = FindVisibleUnpinnedSlotViews();
  if (!first_slot_view || !last_slot_view) {
    return 0;
  }

  const gfx::Rect first_bounds = GetIdealBoundsOf(first_slot_view);
  const gfx::Rect last_bounds = GetIdealBoundsOf(last_slot_view);
  return std::max(0, last_bounds.right() - first_bounds.x());
}

int BraveTabContainer::GetUnpinnedTabsViewportSize() const {
  auto scroll_direction = GetScrollDirection();
  if (!scroll_direction) {
    return 0;
  }

  if (scroll_direction == views::LayoutOrientation::kVertical) {
    return GetUnpinnedTabsViewportHeight();
  }

  // For horizontal tabs, calculate viewport width
  return width() - GetPinnedTabsAreaBoundary();
}

int BraveTabContainer::GetMaxScrollOffset() const {
  auto scroll_direction = GetScrollDirection();
  if (!scroll_direction) {
    return 0;
  }

  int total_size = GetUnpinnedTabsTotalSize();
  if (total_size == 0) {
    return 0;
  }

  if (GetScrollDirection() == views::LayoutOrientation::kHorizontal) {
    // We should subtract the overlap between pinned tabs area and unpinned tabs
    // area
    total_size -= tabs::kHorizontalTabOverlap;
  }

  return std::max(0, total_size - GetUnpinnedTabsViewportSize());
}

void BraveTabContainer::ClampScrollOffset() {
  if (!GetScrollDirection()) {
    return;
  }
  const int clamped = std::clamp(scroll_offset_, 0, GetMaxScrollOffset());
  if (clamped == scroll_offset_) {
    return;
  }
  scroll_offset_ = clamped;
  if (UsesLayerBasedScroll()) {
    ApplyScrollLayerTransform();
    UpdateScrollBarState();
    if (*GetScrollDirection() == views::LayoutOrientation::kHorizontal) {
      horizontal_scroll_offset_changed_callbacks_.Notify();
    }
    return;
  }
  last_layout_size_ = std::nullopt;
  CompleteAnimationAndLayout();
  UpdateScrollBarState();
}

void BraveTabContainer::UpdateClipPathForSlotViews() {
  for (int i = 0; i < GetTabCount(); ++i) {
    Tab* tab = GetTabAtModelIndex(i);
    CHECK(tab);
    tab->SetClipPath({});
  }
  for (auto& [_, group_views] : group_views_) {
    for (auto* view : std::initializer_list<views::View*>{
             group_views->header(), group_views->underline(),
             group_views->highlight(), group_views->drag_underline()}) {
      if (view) {
        view->SetClipPath({});
      }
    }
  }
}

void BraveTabContainer::SetTabPinned(int model_index, TabPinned pinned) {
  TabContainerImpl::SetTabPinned(model_index, pinned);

  GetTabAtModelIndex(model_index)->UpdateInsets();
  UpdatePinnedUnpinnedSeparator();
}

void BraveTabContainer::MoveTab(int from_model_index, int to_model_index) {
  // When pinning a tab requires moving it, the SetTabPinned() won't be called.
  // So we need to update insets here as well.
  TabContainerImpl::MoveTab(from_model_index, to_model_index);
  GetTabAtModelIndex(to_model_index)->UpdateInsets();
  ClampScrollOffset();
  UpdatePinnedUnpinnedSeparator();
}

void BraveTabContainer::OnGroupContentsChanged(
    const tab_groups::TabGroupId& group) {
  TabContainerImpl::OnGroupContentsChanged(group);

  if (GetScrollDirection() && UsesLayerBasedScroll()) {
    ReparentUnpinnedSlotLayers();
  }
  UpdateClipPathForSlotViews();
}

void BraveTabContainer::UpdateTabGroupVisuals(tab_groups::TabGroupId group_id) {
  TabContainerImpl::UpdateTabGroupVisuals(group_id);

  if (GetScrollDirection() && UsesLayerBasedScroll()) {
    ReparentUnpinnedSlotLayers();
  }
  UpdateClipPathForSlotViews();
}

void BraveTabContainer::UpdatePinnedUnpinnedSeparator() {
  // Can be called during the base class' ctor.
  if (!separator_) {
    return;
  }

  if (!ShouldShowVerticalTabs()) {
    separator_->SetVisible(false);
    return;
  }

  const int pinned_tab_count = layout_helper_->GetPinnedTabCount();
  const int total_tab_count = GetTabCount();

  // Only show separator if we have both pinned and unpinned tabs
  if (pinned_tab_count == 0 || pinned_tab_count == total_tab_count) {
    separator_->SetVisible(false);
    return;
  }

  // Position separator between pinned and unpinned tabs
  // GetPinnedTabsAreaBottom() gives the start of unpinned tabs.
  const int separator_y =
      GetPinnedTabsAreaBottom() - tabs::kPinnedUnpinnedSeparatorHeight;
  gfx::Rect separator_bounds(0, separator_y, width(),
                             tabs::kPinnedUnpinnedSeparatorHeight);
  separator_bounds.Inset(
      gfx::Insets::VH(0, tabs::kMarginForVerticalTabContainers));
  separator_->SetBoundsRect(separator_bounds);
  separator_->SetVisible(true);
}

void BraveTabContainer::OnTreeTabsEnabledChanged() {
  CHECK(base::FeatureList::IsEnabled(tabs::kBraveTreeTab));

  layout_helper_->set_use_tree_tabs(*tree_tabs_enabled_);
  if (!ShouldShowVerticalTabs()) {
    return;
  }

  InvalidateIdealBounds();
  InvalidateLayout();
}

bool BraveTabContainer::IsHorizontalScrollableTabStripEnabled() const {
  if (!base::FeatureList::IsEnabled(tabs::kBraveScrollableTabStrip)) {
    return false;
  }

  if (scrollable_horizontal_tab_strip_.GetPrefName().empty()) {
    // Can be null in unit test, as browser object is not available.
    return false;
  }

  return *scrollable_horizontal_tab_strip_;
}

bool BraveTabContainer::IsPinned(const Tab* tab) const {
  CHECK(tab);

  if (tab->data().pinned) {
    return true;
  }

  // Fallback: during tab insertion (StartInsertTabAnimation), SetData() has
  // not been called yet, so data().pinned is always false for newly inserted
  // tabs. Use layout_helper_'s pinned count + view model index instead.
  // This is safe because AddTabs() adds tabs to tabs_view_model_ (via
  // AddTabToViewModel -> layout_helper_->InsertTabAt) before calling
  // StartInsertTabAnimation, so GetIndexOfView(tab) returns the correct index
  // and GetPinnedTabCount() is already accurate when IsPinned() is called.
  const auto pinned_tab_count = layout_helper_->GetPinnedTabCount();
  auto tab_index = tabs_view_model_.GetIndexOfView(tab);
  return tab_index && *tab_index < pinned_tab_count;
}

bool BraveTabContainer::ShouldShowHorizontalScrollButton() const {
  // Scrollable strip behavior comes from kBraveScrollableTabStrip alone; this
  // pref only gates the scroll *buttons* in BraveHorizontalTabStripRegionView.
  if (!base::FeatureList::IsEnabled(tabs::kBraveScrollableTabStrip)) {
    return false;
  }
  const auto direction = GetScrollDirection();
  if (direction != views::LayoutOrientation::kHorizontal) {
    return false;
  }

  return GetMaxScrollOffset() > 0;
}

bool BraveTabContainer::CanScrollTabsStart() const {
  return GetScrollDirection() == views::LayoutOrientation::kHorizontal &&
         scroll_offset_ > 0;
}

bool BraveTabContainer::CanScrollTabsEnd() const {
  return GetScrollDirection() == views::LayoutOrientation::kHorizontal &&
         scroll_offset_ < GetMaxScrollOffset();
}

void BraveTabContainer::ScrollTabsBy(int offset) {
  if (offset == 0) {
    return;
  }

  if (!ShouldShowHorizontalScrollButton()) {
    return;
  }
  HandleScroll(offset);
}

int BraveTabContainer::GetHorizontalTabScrollStep() const {
  const int viewport = GetUnpinnedTabsViewportSize();
  return viewport / 4;
}

int BraveTabContainer::GetScrollOffsetForTesting() const {
  return scroll_offset_;
}

int BraveTabContainer::GetMaxScrollOffsetForTesting() const {
  return GetMaxScrollOffset();
}

void BraveTabContainer::SetScrollOffsetForTesting(int offset) {
  SetScrollOffset(offset);
}

const ui::Layer* BraveTabContainer::GetUnpinnedViewportLayerForTesting() const {
  return unpinned_viewport_layer_.get();
}

bool BraveTabContainer::UsesLayerBasedScroll() const {
  return unpinned_viewport_layer_ && unpinned_scroll_layer_ && layer();
}

void BraveTabContainer::ReorderChildLayers(ui::Layer* parent_layer) {
  if (!UsesLayerBasedScroll() || !unpinned_scroll_layer_) {
    TabContainerImpl::ReorderChildLayers(parent_layer);
    return;
  }

  // When this view's own layer is not |parent_layer|, delegate to the base
  // implementation (handles |layers_above_| / |layers_below_|, which are
  // private on View).
  if (layer() && layer() != parent_layer) {
    TabContainerImpl::ReorderChildLayers(parent_layer);
    return;
  }

  // Unpinned slot layers live under |unpinned_scroll_layer_|, not |layer()|.
  // View::ReorderChildLayers DCHECKs that each child layer's parent matches
  // |parent_layer|. Temporarily move those layers to |layer()|, run the
  // default reorder, then restore the scroll stack.
  ReparentUnpinnedSlotLayersToParentLayer(layer());
  TabContainerImpl::ReorderChildLayers(parent_layer);
  ReparentUnpinnedSlotLayers();
}

gfx::Point BraveTabContainer::AdjustPointForSlotViewHitTest(
    const gfx::Point& point_in_container) const {
  if (!UsesLayerBasedScroll()) {
    return point_in_container;
  }
  const auto direction = *GetScrollDirection();
  const int boundary = GetPinnedTabsAreaBoundary();
  if (direction == views::LayoutOrientation::kVertical) {
    if (point_in_container.y() < boundary) {
      return point_in_container;
    }
    return point_in_container + gfx::Vector2d(0, scroll_offset_);
  }
  if (point_in_container.x() < boundary) {
    return point_in_container;
  }
  return point_in_container + gfx::Vector2d(scroll_offset_, 0);
}

void BraveTabContainer::EnsureScrollLayerStack() {
  if (!GetScrollDirection() || !GetWidget()) {
    return;
  }

  // Step 1: give BraveTabContainer its own LAYER_TEXTURED.
  if (!layer()) {
    SetPaintToLayer(ui::LAYER_TEXTURED);
    if (!layer()) {
      return;
    }
    layer()->SetFillsBoundsOpaquely(false);
    container_layer_added_for_scroll_ = true;
  }

  ui::Layer* parent_layer = layer()->parent();
  cc::Layer* container_cc_layer = layer()->cc_layer_for_testing();
  cc::Layer* parent_cc =
      parent_layer ? parent_layer->cc_layer_for_testing() : nullptr;
  // Walk up the ui::Layer tree to root, logging each ancestor.
  std::string ancestors;
  for (ui::Layer* l = parent_layer; l; l = l->parent()) {
    ancestors += " <- ";
    ancestors += base::StringPrintf("layer=%p type=%d bounds=%s visible=%d",
                                    l, int{l->type()},
                                    l->bounds().ToString().c_str(),
                                    l->visible());
  }
  LOG(ERROR) << "[brave-tabs] EnsureScrollLayerStack container layer=" << layer()
             << " ui_children_count=" << layer()->children().size()
             << " cc_layer=" << container_cc_layer
             << " cc_children_count="
             << (container_cc_layer ? container_cc_layer->children().size() : 0)
             << " cc_hidden="
             << (container_cc_layer ? container_cc_layer->hide_layer_and_subtree()
                                    : false)
             << " parent=" << parent_layer
             << " parent_cc=" << parent_cc
             << " bounds=" << bounds().ToString()
             << " layer_bounds=" << layer()->bounds().ToString()
             << " visible=" << GetVisible()
             << " layer_visible=" << layer()->visible()
             << " opacity=" << layer()->opacity()
             << " parent_layer_bounds="
             << (parent_layer ? parent_layer->bounds().ToString()
                              : std::string("(null)"))
             << " parent_layer_masks_to_bounds="
             << (parent_layer ? parent_layer->GetMasksToBounds() : false)
             << " self_masks_to_bounds=" << layer()->GetMasksToBounds()
             << " ancestors=" << ancestors;

  // Step 2 of incremental re-enable: give each unpinned tab / group view its
  // own LAYER_TEXTURED, parented at BraveTabContainer.layer (no viewport/scroll
  // layers yet). UsesLayerBasedScroll() still returns false, so scroll stays
  // baked into ideal bounds. Tabs paint via compositor instead of the
  // View::Paint walk in BraveTabContainer::PaintChildren.
  const int pinned_count = layout_helper_->GetPinnedTabCount();
  for (int i = 0; i < GetTabCount(); ++i) {
    Tab* tab = GetTabAtModelIndex(i);
    if (IsPinned(tab) || tab->dragging()) {
      // Pinned and dragging tabs keep their upstream layer behavior.
      continue;
    }
    const bool needed_layer = !tab->layer();
    if (!tab->layer()) {
      tab->SetPaintToLayer(ui::LAYER_TEXTURED);
      tab->layer()->SetFillsBoundsOpaquely(false);
      tab->SchedulePaint();
    }
    cc::Layer* tab_cc = tab->layer() ? tab->layer()->cc_layer_for_testing()
                                     : nullptr;
    cc::Layer* container_cc = layer() ? layer()->cc_layer_for_testing()
                                      : nullptr;
    LOG(ERROR) << "[brave-tabs] tab[" << i << "] layer=" << tab->layer()
               << " new=" << needed_layer
               << " parent=" << (tab->layer() ? tab->layer()->parent() : nullptr)
               << " container_layer=" << layer()
               << " type=" << (tab->layer() ? int{tab->layer()->type()} : -1)
               << " cc_drawable="
               << (tab_cc ? tab_cc->draws_content() : false)
               << " cc_layer=" << tab_cc
               << " cc_parent=" << (tab_cc ? tab_cc->parent() : nullptr)
               << " expected_cc_parent=" << container_cc
               << " cc_pos="
               << (tab_cc ? tab_cc->position().ToString() : std::string("(null)"))
               << " cc_bounds="
               << (tab_cc ? tab_cc->bounds().ToString() : std::string("(null)"))
               << " cc_hidden="
               << (tab_cc ? tab_cc->hide_layer_and_subtree() : false)
               << " cc_opacity=" << (tab_cc ? tab_cc->opacity() : -1.f)
               << " bounds=" << tab->bounds().ToString() << " layer_bounds="
               << (tab->layer() ? tab->layer()->bounds().ToString()
                                : std::string("(null)"))
               << " visible=" << tab->GetVisible() << " layer_visible="
               << (tab->layer() ? tab->layer()->visible() : false)
               << " opacity="
               << (tab->layer() ? tab->layer()->opacity() : -1.f);
  }

  for (auto& [group_id, group_views] : group_views_) {
    const std::optional<int> first_in_group =
        controller_->GetFirstTabInGroup(group_id);
    const bool group_is_unpinned =
        first_in_group.has_value() && *first_in_group >= pinned_count;
    if (!group_is_unpinned) {
      continue;
    }
    for (auto* view : std::initializer_list<views::View*>{
             group_views->header(), group_views->underline(),
             group_views->highlight(), group_views->drag_underline()}) {
      if (!view || view->layer()) {
        continue;
      }
      view->SetPaintToLayer(ui::LAYER_TEXTURED);
      view->layer()->SetFillsBoundsOpaquely(false);
      view->SchedulePaint();
    }
  }
}

void BraveTabContainer::TearDownScrollLayerStack() {
  if (!unpinned_viewport_layer_) {
    if (container_layer_added_for_scroll_) {
      DestroyLayer();
      container_layer_added_for_scroll_ = false;
    }
    return;
  }

  ui::Layer* scroll = unpinned_scroll_layer_.get();
  for (int i = 0; i < GetTabCount(); ++i) {
    Tab* tab = GetTabAtModelIndex(i);
    if (tab->layer() && tab->layer()->parent() == scroll) {
      tab->DestroyLayer();
    }
  }

  for (auto& [_, group_views] : group_views_) {
    for (auto* view : std::initializer_list<views::View*>{
             group_views->header(), group_views->underline(),
             group_views->highlight(), group_views->drag_underline()}) {
      if (view && view->layer() && view->layer()->parent() == scroll) {
        view->DestroyLayer();
      }
    }
  }

  if (scroll->parent() == unpinned_viewport_layer_.get()) {
    unpinned_viewport_layer_->Remove(scroll);
  }
  unpinned_scroll_layer_.reset();

  ui::Layer* root = layer();
  if (root && unpinned_viewport_layer_->parent() == root) {
    root->Remove(unpinned_viewport_layer_.get());
  }
  unpinned_viewport_layer_.reset();

  if (container_layer_added_for_scroll_) {
    DestroyLayer();
    container_layer_added_for_scroll_ = false;
  }
}

void BraveTabContainer::UpdateScrollLayerBoundsAndClip() {
  if (!UsesLayerBasedScroll()) {
    return;
  }

  const auto direction = *GetScrollDirection();
  const int boundary = GetPinnedTabsAreaBoundary();
  gfx::Rect viewport;
  if (direction == views::LayoutOrientation::kVertical) {
    viewport =
        gfx::Rect(0, boundary, width(), std::max(0, height() - boundary));
  } else {
    viewport =
        gfx::Rect(boundary, 0, std::max(0, width() - boundary), height());
  }

  unpinned_viewport_layer_->SetBounds(viewport);

  // Place |unpinned_scroll_layer_| so its content origin matches this view's
  // layer origin. Slot views compute their layer position from
  // GetMirroredPosition() (in this view's coord space) via
  // View::MoveLayerToParent, so the scroll layer must compensate for the
  // viewport's offset within this view, otherwise tabs render below the
  // pinned-area boundary and get clipped out of the viewport.
  gfx::Rect scroll_bounds(width(), height());
  if (direction == views::LayoutOrientation::kVertical) {
    scroll_bounds.set_origin(gfx::Point(0, -boundary));
  } else {
    scroll_bounds.set_origin(gfx::Point(-boundary, 0));
  }
  unpinned_scroll_layer_->SetBounds(scroll_bounds);
}

void BraveTabContainer::ApplyScrollLayerTransform() {
  if (!unpinned_scroll_layer_) {
    return;
  }
  gfx::Transform transform;
  const auto direction = GetScrollDirection();
  if (direction == views::LayoutOrientation::kVertical) {
    transform.Translate(0, -scroll_offset_);
  } else if (direction == views::LayoutOrientation::kHorizontal) {
    transform.Translate(-scroll_offset_, 0);
  }
  unpinned_scroll_layer_->SetTransform(transform);
}

void BraveTabContainer::ReparentUnpinnedSlotLayersToParentLayer(
    ui::Layer* target_parent) {
  DCHECK(target_parent);
  if (!UsesLayerBasedScroll()) {
    return;
  }

  ui::Layer* scroll = unpinned_scroll_layer_.get();
  const int pinned_count = layout_helper_->GetPinnedTabCount();

  for (int i = 0; i < GetTabCount(); ++i) {
    Tab* tab = GetTabAtModelIndex(i);
    if (IsPinned(tab)) {
      continue;
    }
    if (!tab->layer() || tab->layer()->parent() != scroll) {
      continue;
    }
    ReparentSlotViewLayerToParent(tab, target_parent);
  }

  for (auto& [group_id, group_views] : group_views_) {
    const std::optional<int> first_in_group =
        controller_->GetFirstTabInGroup(group_id);
    const bool group_is_unpinned =
        first_in_group.has_value() && *first_in_group >= pinned_count;
    if (!group_is_unpinned) {
      continue;
    }

    for (auto* view : std::initializer_list<views::View*>{
             group_views->header(), group_views->underline(),
             group_views->highlight(), group_views->drag_underline()}) {
      if (!view || !view->layer() || view->layer()->parent() != scroll) {
        continue;
      }
      ReparentSlotViewLayerToParent(view, target_parent);
    }
  }
}

void BraveTabContainer::ReparentUnpinnedSlotLayers() {
  if (!UsesLayerBasedScroll()) {
    return;
  }

  ui::Layer* scroll = unpinned_scroll_layer_.get();
  const int pinned_count = layout_helper_->GetPinnedTabCount();

  for (int i = 0; i < GetTabCount(); ++i) {
    Tab* tab = GetTabAtModelIndex(i);
    if (IsPinned(tab)) {
      if (tab->layer() && tab->layer()->parent() == scroll) {
        tab->DestroyLayer();
      }
      continue;
    }
    if (tab->dragging()) {
      continue;
    }
    if (!tab->layer()) {
      tab->SetPaintToLayer(ui::LAYER_TEXTURED);
      tab->layer()->SetFillsBoundsOpaquely(false);
    }
    ReparentSlotViewLayerForScroll(tab, scroll);
    tab->SchedulePaint();
  }

  for (auto& [group_id, group_views] : group_views_) {
    const std::optional<int> first_in_group =
        controller_->GetFirstTabInGroup(group_id);
    const bool group_is_unpinned =
        first_in_group.has_value() && *first_in_group >= pinned_count;

    for (auto* view : std::initializer_list<views::View*>{
             group_views->header(), group_views->underline(),
             group_views->highlight(), group_views->drag_underline()}) {
      if (!view) {
        continue;
      }
      if (!group_is_unpinned) {
        if (view->layer() && view->layer()->parent() == scroll) {
          view->DestroyLayer();
        }
        continue;
      }
      if (!view->layer()) {
        view->SetPaintToLayer(ui::LAYER_TEXTURED);
        view->layer()->SetFillsBoundsOpaquely(false);
      }
      ReparentSlotViewLayerForScroll(view, scroll);
      view->SchedulePaint();
    }
  }
}

BEGIN_METADATA(BraveTabContainer)
END_METADATA
