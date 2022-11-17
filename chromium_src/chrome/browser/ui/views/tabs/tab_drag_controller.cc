/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_drag_controller.h"

#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/views/view.h"
#include "ui/views/widget/root_view.h"
#include "ui/views/widget/widget.h"

#define TabDragController TabDragControllerChromium

// Wraps function calls so that they can work with a child NativeWindow as well.
#define GetBrowserViewForNativeWindow(local_window)         \
  GetBrowserViewForNativeWindow(                            \
      views::Widget::GetWidgetForNativeWindow(local_window) \
          ->GetTopLevelWidget()                             \
          ->GetNativeWindow())
#define ConvertPointToWidget(view, point) \
  ConvertPointToScreen(view, point);      \
  views::View::ConvertPointFromScreen(    \
      view->GetWidget()->GetTopLevelWidget()->GetRootView(), point)
#define GetRestoredBounds GetTopLevelWidget()->GetRestoredBounds

#include "src/chrome/browser/ui/views/tabs/tab_drag_controller.cc"

#undef GetRestoredBounds
#undef ConvertPointToWidget
#undef GetBrowserViewForNativeWindow
#undef TabDragController

#include "brave/browser/ui/views/tabs/features.h"

TabDragController::TabDragController() = default;

TabDragController::~TabDragController() = default;

void TabDragController::Init(TabDragContext* source_context,
                             TabSlotView* source_view,
                             const std::vector<TabSlotView*>& dragging_views,
                             const gfx::Point& mouse_offset,
                             int source_view_offset,
                             ui::ListSelectionModel initial_selection_model,
                             ui::mojom::DragEventSource event_source) {
  TabDragControllerChromium::Init(source_context, source_view, dragging_views,
                                  mouse_offset, source_view_offset,
                                  initial_selection_model, event_source);
  auto* widget = source_view->GetWidget()->GetTopLevelWidget();
  DCHECK(widget);
  const auto* browser =
      BrowserView::GetBrowserViewForNativeWindow(widget->GetNativeWindow())
          ->browser();
  is_showing_vertical_tabs_ = tabs::features::ShouldShowVerticalTabs(browser);

  if (!is_showing_vertical_tabs_)
    return;

  // Adjust coordinate for vertical mode.
  source_view_offset = mouse_offset.y();
  start_point_in_screen_ = gfx::Point(mouse_offset.x(), source_view_offset);
  views::View::ConvertPointToScreen(source_view, &start_point_in_screen_);

  last_point_in_screen_ = start_point_in_screen_;
  last_move_screen_loc_ = start_point_in_screen_.y();
}

gfx::Point TabDragController::GetAttachedDragPoint(
    const gfx::Point& point_in_screen) {
  if (!is_showing_vertical_tabs_)
    return TabDragControllerChromium::GetAttachedDragPoint(point_in_screen);

  DCHECK(attached_context_);  // The tab must be attached.

  gfx::Point tab_loc(point_in_screen);
  views::View::ConvertPointFromScreen(attached_context_, &tab_loc);
  const int y = tab_loc.y() - mouse_offset_.y();
  return gfx::Point(0, std::max(0, y));
}

void TabDragController::MoveAttached(const gfx::Point& point_in_screen,
                                     bool just_attached) {
  gfx::Point new_point_in_screen = point_in_screen;
  if (is_showing_vertical_tabs_) {
    // As MoveAttached() compare point_in_screen.x() and last_move_screen_loc_,
    // override x with y so that calculate offset for vertical mode.
    new_point_in_screen.set_x(new_point_in_screen.y());
  }

  TabDragControllerChromium::MoveAttached(new_point_in_screen, just_attached);
}

absl::optional<tab_groups::TabGroupId>
TabDragController::GetTabGroupForTargetIndex(const std::vector<int>& selected) {
  auto group_id =
      TabDragControllerChromium::GetTabGroupForTargetIndex(selected);
  if (group_id.has_value() || !is_showing_vertical_tabs_)
    return group_id;

  // We have corner cases where the chromium code can't handle.
  // When former group and latter group of selected tabs are different, Chromium
  // calculates the target index based on the x coordinate. So we need to check
  // this again based on y coordinate.
  const auto previous_tab_index = selected.front() - 1;

  const TabStripModel* attached_model = attached_context_->GetTabStripModel();
  const absl::optional<tab_groups::TabGroupId> former_group =
      attached_model->GetTabGroupForTab(previous_tab_index);
  const absl::optional<tab_groups::TabGroupId> latter_group =
      attached_model->GetTabGroupForTab(selected.back() + 1);
  // We assume that Chromium can handle it when former and latter group are
  // same. So just return here.
  if (former_group == latter_group)
    return group_id;

  const auto top_edge =
      previous_tab_index >= 0
          ? attached_context_->GetTabAt(previous_tab_index)->bounds().bottom()
          : 0;
  const auto first_selected_tab_y =
      attached_context_->GetTabAt(selected.front())->bounds().y();
  if (former_group.has_value() &&
      !attached_model->IsGroupCollapsed(*former_group)) {
    if (first_selected_tab_y <= top_edge)
      return former_group;
  }

  if (latter_group.has_value() &&
      !attached_model->IsGroupCollapsed(*latter_group)) {
    if (first_selected_tab_y >= top_edge)
      return latter_group;
  }

  return group_id;
}

views::Widget* TabDragController::GetAttachedBrowserWidget() {
  return TabDragControllerChromium::GetAttachedBrowserWidget()
      ->GetTopLevelWidget();
}

TabDragController::Liveness TabDragController::GetLocalProcessWindow(
    const gfx::Point& screen_point,
    bool exclude_dragged_view,
    gfx::NativeWindow* window) {
  if (is_showing_vertical_tabs_ && exclude_dragged_view) {
    // In this case, we need to exclude a widget for vertical tab strip too.
    std::set<gfx::NativeWindow> exclude;
    auto* dragged_widget = attached_context_->GetWidget();
    if (dragged_widget) {
      exclude.insert(dragged_widget->GetNativeWindow());
      exclude.insert(dragged_widget->GetTopLevelWidget()->GetNativeWindow());
    }
    base::WeakPtr<TabDragControllerChromium> ref(weak_factory_.GetWeakPtr());
    *window =
        window_finder_->GetLocalProcessWindowAtPoint(screen_point, exclude);
    return ref ? Liveness::ALIVE : Liveness::DELETED;
  }

  return TabDragControllerChromium::GetLocalProcessWindow(
      screen_point, exclude_dragged_view, window);
}

void TabDragController::DetachAndAttachToNewContext(
    ReleaseCapture release_capture,
    TabDragContext* target_context,
    const gfx::Point& point_in_screen,
    bool set_capture) {
  TabDragControllerChromium::DetachAndAttachToNewContext(
      release_capture, target_context, point_in_screen, set_capture);
  if (!is_showing_vertical_tabs_)
    return;

  auto* browser_view =
      static_cast<BraveBrowserView*>(BrowserView::GetBrowserViewForNativeWindow(
          GetAttachedBrowserWidget()->GetNativeWindow()));
  DCHECK(browser_view);

  auto* widget_delegate_view =
      browser_view->vertical_tab_strip_widget_delegate_view();
  DCHECK(widget_delegate_view);

  auto* region_view = widget_delegate_view->vertical_tab_strip_region_view();
  DCHECK(region_view);

  vertical_tab_state_resetter_ = region_view->ExpandTabStripForDragging();
  // Relayout tabs with expanded bounds.
  attached_context_->ForceLayout();

  std::vector<TabSlotView*> views(drag_data_.size());
  for (size_t i = 0; i < drag_data_.size(); ++i)
    views[i] = drag_data_[i].attached_view;

  attached_context_->LayoutDraggedViewsAt(
      views, source_view_drag_data()->attached_view, point_in_screen,
      initial_move_);
}

gfx::Rect TabDragController::CalculateNonMaximizedDraggedBrowserBounds(
    views::Widget* widget,
    const gfx::Point& point_in_screen) {
  // This method is called when dragging all tabs and moving window.
  auto bounds =
      TabDragControllerChromium::CalculateNonMaximizedDraggedBrowserBounds(
          widget, point_in_screen);
#if BUILDFLAG(IS_MAC)
  // According to what's been observed, this only needed on Mac. Per platform,
  // window management mechanism is different so this could happen.
  if (is_showing_vertical_tabs_) {
    bounds.Offset(GetVerticalTabStripWidgetOffset());
  }
#endif

  return bounds;
}

gfx::Rect TabDragController::CalculateDraggedBrowserBounds(
    TabDragContext* source,
    const gfx::Point& point_in_screen,
    std::vector<gfx::Rect>* drag_bounds) {
  // This method is called when creating new browser by detaching tabs.
  auto bounds = TabDragControllerChromium::CalculateDraggedBrowserBounds(
      source, point_in_screen, drag_bounds);
  if (is_showing_vertical_tabs_) {
    // Revert back y coordinate adjustment done by Chromium impl.
    bounds.set_y(point_in_screen.y());

    // Adjust y coordinate so that dragged tabs are under cursor.
    auto* browser_view = static_cast<BraveBrowserView*>(
        BrowserView::GetBrowserViewForNativeWindow(
            GetAttachedBrowserWidget()->GetNativeWindow()));
    DCHECK(browser_view);

    auto* widget_delegate_view =
        browser_view->vertical_tab_strip_widget_delegate_view();
    DCHECK(widget_delegate_view);

    bounds.Offset({0, GetVerticalTabStripWidgetOffset().y()});
    bounds.Offset(-widget_delegate_view->vertical_tab_strip_region_view()
                       ->GetOffsetForDraggedTab());
  }

  return bounds;
}

gfx::Vector2d TabDragController::GetVerticalTabStripWidgetOffset() {
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
