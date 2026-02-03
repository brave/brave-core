/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/dragging/tab_drag_controller.h"

#include <algorithm>
#include <optional>
#include <set>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_context.h"
#include "chrome/browser/ui/views/tabs/window_finder.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_group.h"
#include "ui/views/view_utils.h"

namespace {

int GetXCoordinateAdjustmentForMultiSelectedTabs(
    const std::vector<TabSlotView*>& dragged_views,
    int source_view_index) {
  if (dragged_views.at(source_view_index)->GetTabSlotViewType() ==
          TabSlotView::ViewType::kTabGroupHeader ||
      source_view_index == 0) {
    return 0;
  }

  // When selecting multiple tabs, the x coordinate is not exactly same with
  // where it was pressed. Because Chromium adjust it by the width of previous
  // tabs(See TabStrip::GetSizeNeededForViews() and its call sites). But we
  // don't want this behavior. With this adjustment selecting multiple tabs
  // without dragging make tabs or the window jump around by the amount of the
  // width of other tabs. https://github.com/brave/brave-browser/issues/29465
  return TabStrip::GetSizeNeededForViews(std::vector(
      dragged_views.begin(), dragged_views.begin() + source_view_index));
}

}  // namespace

BraveTabDragController::BraveTabDragController() = default;

BraveTabDragController::~BraveTabDragController() = default;

BraveTabDragController::Liveness BraveTabDragController::Init(
    TabDragContext* source_context,
    TabSlotView* source_view,
    const std::vector<TabSlotView*>& dragging_views,
    const gfx::Point& offset_from_first_dragged_view,
    const gfx::Point& offset_from_source_view,
    ui::ListSelectionModel initial_selection_model,
    ui::mojom::DragEventSource event_source) {
  if (TabDragController::Init(source_context, source_view, dragging_views,
                              offset_from_first_dragged_view,
                              offset_from_source_view, initial_selection_model,
                              event_source) ==
      BraveTabDragController::BraveTabDragController::Liveness::kDeleted) {
    return BraveTabDragController::BraveTabDragController::Liveness::kDeleted;
  }

  offset_from_first_dragged_view_ = offset_from_first_dragged_view;

  auto* widget = source_view->GetWidget();
  DCHECK(widget);
  const auto* browser =
      BrowserView::GetBrowserViewForNativeWindow(widget->GetNativeWindow())
          ->browser();

  if (base::FeatureList::IsEnabled(tabs::kBraveSharedPinnedTabs) &&
      browser->profile()->GetPrefs()->GetBoolean(
          brave_tabs::kSharedPinnedTab)) {
    if (std::ranges::any_of(dragging_views, [](TabSlotView* slot_view) {
          // We don't allow sharable pinned tabs to be detached.
          return slot_view->GetTabSlotViewType() ==
                     TabSlotView::ViewType::kTab &&
                 views::AsViewClass<Tab>(slot_view)->data().pinned;
        })) {
      detach_behavior_ = DetachBehavior::kNotDetachable;
    }
  }

  is_showing_vertical_tabs_ = tabs::utils::ShouldShowBraveVerticalTabs(browser);

  if (!is_showing_vertical_tabs_) {
    return BraveTabDragController::BraveTabDragController::Liveness::kAlive;
  }

  // Update IsMaximized and IsFullscreen states for vertical mode.
  auto* top_level_widget = widget->GetTopLevelWidget();
  DCHECK(top_level_widget);
  was_source_maximized_ = top_level_widget->IsMaximized();
  was_source_fullscreen_ = top_level_widget->IsFullscreen();

  // Adjust coordinate for vertical mode.
  const int x = offset_from_first_dragged_view.x() -
                GetXCoordinateAdjustmentForMultiSelectedTabs(
                    dragging_views, drag_data_.source_view_index_);
  start_point_in_screen_ = gfx::Point(x, offset_from_first_dragged_view.y());
  views::View::ConvertPointToScreen(source_view, &start_point_in_screen_);

  last_point_in_screen_ = start_point_in_screen_;
  return BraveTabDragController::BraveTabDragController::Liveness::kAlive;
}

gfx::Vector2d BraveTabDragController::CalculateWindowDragOffset() {
  gfx::Vector2d offset = TabDragController::CalculateWindowDragOffset();
  if (!is_showing_vertical_tabs_) {
    return offset;
  }

  // Re-calculate offset as above result is based on vertical tab widget.
  // Convert it based on browser window widget(top level widget).
  gfx::Point new_offset(offset.x(), offset.y());
  views::View::ConvertPointFromWidget(attached_context_, &new_offset);
  views::View::ConvertPointToScreen(attached_context_, &new_offset);
  views::View::ConvertPointFromScreen(
      attached_context_->GetWidget()->GetTopLevelWidget()->GetRootView(),
      &new_offset);
  return new_offset.OffsetFromOrigin();
}

void BraveTabDragController::StartDraggingTabsSession(
    bool initial_move,
    gfx::Point start_point_in_screen) {
  TabDragController::StartDraggingTabsSession(initial_move,
                                              start_point_in_screen);
  CHECK(dragging_tabs_session_);
  dragging_tabs_session_->set_mouse_y_offset(
      offset_from_first_dragged_view_.y());
  dragging_tabs_session_->set_is_showing_vertical_tabs(
      is_showing_vertical_tabs_);
}

views::Widget* BraveTabDragController::GetAttachedBrowserWidget() {
  auto* widget = TabDragController::GetAttachedBrowserWidget();
  if (!is_showing_vertical_tabs_) {
    return widget;
  }

  // As vertical tab strip is attached to child widget of browser widget,
  // we should return top level widget.
  DCHECK(widget);
  auto* top_level_widget = widget->GetTopLevelWidget();
  DCHECK(top_level_widget);
  return top_level_widget;
}

BraveTabDragController::Liveness BraveTabDragController::GetLocalProcessWindow(
    const gfx::Point& screen_point,
    bool exclude_dragged_view,
    gfx::NativeWindow* window) {
  if (is_showing_vertical_tabs_ && exclude_dragged_view) {
    // In this case, we need to exclude a widget for vertical tab strip too.
    std::set<gfx::NativeWindow> exclude;
    auto* dragged_widget = attached_context_->GetWidget();
    DCHECK(dragged_widget);
    if (dragged_widget) {
      exclude.insert(dragged_widget->GetNativeWindow());
      auto* top_level_widget = dragged_widget->GetTopLevelWidget();
      DCHECK(top_level_widget);
      exclude.insert(top_level_widget->GetNativeWindow());
    }
    base::WeakPtr<TabDragController> ref(weak_factory_.GetWeakPtr());
    *window =
        window_finder_->GetLocalProcessWindowAtPoint(screen_point, exclude);
    return ref ? BraveTabDragController::Liveness::kAlive
               : BraveTabDragController::Liveness::kDeleted;
  }

  return TabDragController::GetLocalProcessWindow(screen_point,
                                                  exclude_dragged_view, window);
}

void BraveTabDragController::DetachAndAttachToNewContext(
    ReleaseCapture release_capture,
    TabDragContext* target_context) {
  if (!is_showing_vertical_tabs_) {
    TabDragController::DetachAndAttachToNewContext(release_capture,
                                                   target_context);
    return;
  }

  auto get_region_view = [this] {
    auto* browser_view = static_cast<BraveBrowserView*>(
        BrowserView::GetBrowserViewForNativeWindow(
            GetAttachedBrowserWidget()->GetNativeWindow()));
    DCHECK(browser_view);

    auto* widget_delegate_view =
        browser_view->vertical_tab_strip_widget_delegate_view();
    DCHECK(widget_delegate_view);

    auto* region_view = widget_delegate_view->vertical_tab_strip_region_view();
    DCHECK(region_view);

    return region_view;
  };

  if (!vertical_tab_state_resetter_) {
    // In case it was the very first drag-and-drop source, this could be null.
    // But we also still need to collapse it when detaching tabs in to new
    // browser. So call ExpandTabStripForDragging() so that it can be collapsed
    // in the same manner.
    auto* region_view = get_region_view();
    vertical_tab_state_resetter_ = region_view->ExpandTabStripForDragging();
  }

  TabDragController::DetachAndAttachToNewContext(release_capture,
                                                 target_context);

  auto* region_view = get_region_view();

  vertical_tab_state_resetter_ = region_view->ExpandTabStripForDragging();
  // Relayout tabs with expanded bounds.
  attached_context_->GetPositioningDelegate()->ForceLayout();
}

gfx::Vector2d BraveTabDragController::GetVerticalTabStripWidgetOffset() {
  auto* browser_widget = GetAttachedBrowserWidget();
  DCHECK(browser_widget);
  auto browser_widget_bounds = browser_widget->GetWindowBoundsInScreen();

  auto* browser_view =
      static_cast<BraveBrowserView*>(BrowserView::GetBrowserViewForNativeWindow(
          browser_widget->GetNativeWindow()));
  DCHECK(browser_view);

  auto* tabstrip_widget =
      browser_view->vertical_tab_strip_widget_delegate_view()->GetWidget();
  DCHECK(tabstrip_widget);
  auto tabstrip_widget_bounds = tabstrip_widget->GetWindowBoundsInScreen();

  return browser_widget_bounds.origin() - tabstrip_widget_bounds.origin();
}

void BraveTabDragController::RestoreAttachedWindowForDrag() {
  if (!is_showing_vertical_tabs_) {
    TabDragController::RestoreAttachedWindowForDrag();
    return;
  }

  const gfx::Size restored_size = CalculateDraggedWindowSize(attached_context_);

  views::Widget* widget = GetAttachedBrowserWidget();
  widget->SetVisibilityChangedAnimationsEnabled(false);
  widget->Restore();
  widget->SetVisibilityChangedAnimationsEnabled(true);

  widget->SetSize(restored_size);
}
