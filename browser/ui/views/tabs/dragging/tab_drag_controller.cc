/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/dragging/tab_drag_controller.h"

#include <algorithm>
#include <optional>
#include <set>
#include <utility>

#include "base/feature_list.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/tabs/split_view_browser_data.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_region_view.h"
#include "brave/browser/ui/views/frame/vertical_tab_strip_widget_delegate_view.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/tab_group.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_context.h"
#include "chrome/browser/ui/views/tabs/window_finder.h"
#include "components/prefs/pref_service.h"
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

TabDragController::TabDragController() = default;

TabDragController::~TabDragController() = default;

TabDragController::Liveness TabDragController::Init(
    TabDragContext* source_context,
    TabSlotView* source_view,
    const std::vector<TabSlotView*>& dragging_views,
    const gfx::Point& mouse_offset,
    int source_view_offset,
    ui::ListSelectionModel initial_selection_model,
    ui::mojom::DragEventSource event_source) {
  if (TabDragControllerChromium::Init(
          source_context, source_view, dragging_views, mouse_offset,
          source_view_offset, initial_selection_model,
          event_source) == TabDragController::Liveness::DELETED) {
    return TabDragController::Liveness::DELETED;
  }

  mouse_offset_ = mouse_offset;

  auto* widget = source_view->GetWidget();
  DCHECK(widget);
  const auto* browser =
      BrowserView::GetBrowserViewForNativeWindow(widget->GetNativeWindow())
          ->browser();

  if (base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs) &&
      browser->profile()->GetPrefs()->GetBoolean(
          brave_tabs::kSharedPinnedTab)) {
    if (std::ranges::any_of(dragging_views, [](TabSlotView* slot_view) {
          // We don't allow sharable pinned tabs to be detached.
          return slot_view->GetTabSlotViewType() ==
                     TabSlotView::ViewType::kTab &&
                 views::AsViewClass<Tab>(slot_view)->data().pinned;
        })) {
      detach_behavior_ = NOT_DETACHABLE;
    }
  }

  is_showing_vertical_tabs_ = tabs::utils::ShouldShowVerticalTabs(browser);

  if (!is_showing_vertical_tabs_) {
    return TabDragController::Liveness::ALIVE;
  }

  // Adjust coordinate for vertical mode.
  const int x =
      mouse_offset.x() - GetXCoordinateAdjustmentForMultiSelectedTabs(
                             dragging_views, drag_data_.source_view_index_);
  source_view_offset = mouse_offset.y();
  start_point_in_screen_ = gfx::Point(x, source_view_offset);
  views::View::ConvertPointToScreen(source_view, &start_point_in_screen_);

  last_point_in_screen_ = start_point_in_screen_;
  return TabDragController::Liveness::ALIVE;
}

gfx::Vector2d TabDragController::CalculateWindowDragOffset() {
  gfx::Vector2d offset = TabDragControllerChromium::CalculateWindowDragOffset();
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

void TabDragController::StartDraggingTabsSession(
    bool initial_move,
    gfx::Point start_point_in_screen) {
  TabDragControllerChromium::StartDraggingTabsSession(initial_move,
                                                      start_point_in_screen);
  CHECK(dragging_tabs_session_);
  dragging_tabs_session_->set_mouse_y_offset(mouse_offset_.y());
  dragging_tabs_session_->set_is_showing_vertical_tabs(
      is_showing_vertical_tabs_);
}

views::Widget* TabDragController::GetAttachedBrowserWidget() {
  auto* widget = TabDragControllerChromium::GetAttachedBrowserWidget();
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

TabDragController::Liveness TabDragController::GetLocalProcessWindow(
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
    TabDragContext* target_context) {
  auto* browser_widget = GetAttachedBrowserWidget();
  auto* browser = BrowserView::GetBrowserViewForNativeWindow(
                      browser_widget->GetNativeWindow())
                      ->browser();
  SplitViewBrowserData* old_split_view_browser_data =
      browser->GetFeatures().split_view_browser_data();
  if (old_split_view_browser_data) {
    std::vector<tabs::TabHandle> tabs;
    auto* tab_strip_model = browser->tab_strip_model();
    DCHECK_EQ(tab_strip_model, attached_context_->GetTabStripModel());
    auto drag_data = base::span(drag_data_.tab_drag_data_)
                         .subspan(static_cast<size_t>(first_tab_index()));
    for (const auto& data : drag_data) {
      tabs.push_back(tab_strip_model
                         ->GetTabAtIndex(tab_strip_model->GetIndexOfWebContents(
                             data.contents))
                         ->GetHandle());
    }
    old_split_view_browser_data->TabsWillBeAttachedToNewBrowser(tabs);
  }

  if (!is_showing_vertical_tabs_) {
    TabDragControllerChromium::DetachAndAttachToNewContext(release_capture,
                                                           target_context);

    if (old_split_view_browser_data) {
      auto* new_browser = BrowserView::GetBrowserViewForNativeWindow(
                              GetAttachedBrowserWidget()->GetNativeWindow())
                              ->browser();
      old_split_view_browser_data->TabsAttachedToNewBrowser(
          new_browser->GetFeatures().split_view_browser_data());
    }
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

  TabDragControllerChromium::DetachAndAttachToNewContext(release_capture,
                                                         target_context);

  auto* region_view = get_region_view();

  vertical_tab_state_resetter_ = region_view->ExpandTabStripForDragging();
  // Relayout tabs with expanded bounds.
  attached_context_->ForceLayout();

  std::vector<TabSlotView*> views(drag_data_.tab_drag_data_.size());
  for (size_t i = 0; i < drag_data_.tab_drag_data_.size(); ++i) {
    views[i] = drag_data_.tab_drag_data_[i].attached_view.get();
  }

  attached_context_->LayoutDraggedViewsAt(
      std::move(views), drag_data_.source_view_drag_data()->attached_view,
      GetCursorScreenPoint(), false);

  if (old_split_view_browser_data) {
    auto* new_browser = BrowserView::GetBrowserViewForNativeWindow(
                            GetAttachedBrowserWidget()->GetNativeWindow())
                            ->browser();
    old_split_view_browser_data->TabsAttachedToNewBrowser(
        new_browser->GetFeatures().split_view_browser_data());
  }
}

[[nodiscard]] TabDragController::Liveness TabDragController::ContinueDragging(
    const gfx::Point& point_in_screen) {
  auto* browser_widget = GetAttachedBrowserWidget();
  auto* browser = BrowserView::GetBrowserViewForNativeWindow(
                      browser_widget->GetNativeWindow())
                      ->browser();
  SplitViewBrowserData* split_view_browser_data =
      browser->GetFeatures().split_view_browser_data();
  if (!split_view_browser_data) {
    return TabDragControllerChromium::ContinueDragging(point_in_screen);
  }

  auto weak = weak_factory_.GetWeakPtr();
  const auto liveness =
      TabDragControllerChromium::ContinueDragging(point_in_screen);

  if (!weak) {
    // In DragBrowserToNewTabStrip() could delete itself, so we need to check
    // it's still alive first.
    return liveness;
  }

  if (!attached_context_) {
    // This is when drag session ends.
    on_tab_drag_ended_closure_.RunAndReset();
    return liveness;
  }

  const bool is_dragging_tabs = current_state_ == DragState::kDraggingTabs;
  if (is_dragging_tabs) {
    on_tab_drag_ended_closure_ = split_view_browser_data->TabDragStarted();
  } else {
    // This is a case where tabs are detached into new window and enters.
    // Notifies that drag session ended to the old browser.
    on_tab_drag_ended_closure_.RunAndReset();
  }

  return liveness;
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
