/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/tab_drag_controller.h"

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
#include "chrome/browser/ui/tabs/tab_group.h"
#include "chrome/browser/ui/tabs/tab_group_model.h"
#include "chrome/browser/ui/views/tabs/tab_drag_context.h"
#include "chrome/browser/ui/views/tabs/window_finder.h"
#include "components/prefs/pref_service.h"
#include "ui/views/view_utils.h"

namespace {

int GetXCoordinateAdjustmentForMultiSelectedTabs(
    const std::vector<raw_ptr<TabSlotView, VectorExperimental>>& dragged_views,
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
    const std::vector<raw_ptr<TabSlotView, VectorExperimental>>& dragging_views,
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

  auto* widget = source_view->GetWidget();
  DCHECK(widget);
  const auto* browser =
      BrowserView::GetBrowserViewForNativeWindow(widget->GetNativeWindow())
          ->browser();

  if (base::FeatureList::IsEnabled(tabs::features::kBraveSharedPinnedTabs) &&
      browser->profile()->GetPrefs()->GetBoolean(
          brave_tabs::kSharedPinnedTab)) {
    if (base::ranges::any_of(dragging_views, [](TabSlotView* slot_view) {
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
  const int x = mouse_offset.x() - GetXCoordinateAdjustmentForMultiSelectedTabs(
                                       dragging_views, source_view_index_);
  source_view_offset = mouse_offset.y();
  start_point_in_screen_ = gfx::Point(x, source_view_offset);
  views::View::ConvertPointToScreen(source_view, &start_point_in_screen_);

  last_point_in_screen_ = start_point_in_screen_;
  return TabDragController::Liveness::ALIVE;
}

gfx::Point TabDragController::GetAttachedDragPoint(
    const gfx::Point& point_in_screen) {
  if (!is_showing_vertical_tabs_) {
    return TabDragControllerChromium::GetAttachedDragPoint(point_in_screen);
  }

  DCHECK(attached_context_);  // The tab must be attached.

  gfx::Point tab_loc(point_in_screen);
  views::View::ConvertPointFromScreen(attached_context_, &tab_loc);
  const int x = drag_data_.front().pinned ? tab_loc.x() - mouse_offset_.x() : 0;
  const int y = tab_loc.y() - mouse_offset_.y();
  return {x, y};
}

void TabDragController::MoveAttached(const gfx::Point& point_in_screen,
                                     bool just_attached) {
  TabDragControllerChromium::MoveAttached(point_in_screen, just_attached);
  if (!is_showing_vertical_tabs_) {
    return;
  }

  // Unlike upstream, We always update coordinate, as we use y coordinate. Since
  // we don't have threshold there's no any harm for this.
  gfx::Point point_in_attached_context = point_in_screen;
  views::View::ConvertPointFromScreen(attached_context_,
                                      &point_in_attached_context);
  last_move_attached_context_loc_ = point_in_attached_context.y();
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
    TabDragContext* target_context,
    const gfx::Point& point_in_screen,
    bool set_capture) {
  auto* browser_widget = GetAttachedBrowserWidget();
  auto* browser = BrowserView::GetBrowserViewForNativeWindow(
                      browser_widget->GetNativeWindow())
                      ->browser();
  SplitViewBrowserData* old_split_view_browser_data =
      SplitViewBrowserData::FromBrowser(browser);
  if (old_split_view_browser_data) {
    std::vector<tabs::TabHandle> tabs;
    auto* tab_strip_model = browser->tab_strip_model();
    DCHECK_EQ(tab_strip_model, attached_context_->GetTabStripModel());
    for (size_t i = first_tab_index(); i < drag_data_.size(); ++i) {
      tabs.push_back(tab_strip_model->GetTabHandleAt(
          tab_strip_model->GetIndexOfWebContents(drag_data_[i].contents)));
    }
    old_split_view_browser_data->TabsWillBeAttachedToNewBrowser(tabs);
  }

  if (!is_showing_vertical_tabs_) {
    TabDragControllerChromium::DetachAndAttachToNewContext(
        release_capture, target_context, point_in_screen, set_capture);

    if (old_split_view_browser_data) {
      auto* new_browser = BrowserView::GetBrowserViewForNativeWindow(
                              GetAttachedBrowserWidget()->GetNativeWindow())
                              ->browser();
      old_split_view_browser_data->TabsAttachedToNewBrowser(new_browser);
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

  TabDragControllerChromium::DetachAndAttachToNewContext(
      release_capture, target_context, point_in_screen, set_capture);

  auto* region_view = get_region_view();

  vertical_tab_state_resetter_ = region_view->ExpandTabStripForDragging();
  // Relayout tabs with expanded bounds.
  attached_context_->ForceLayout();

  std::vector<raw_ptr<TabSlotView, VectorExperimental>> views(
      drag_data_.size());
  for (size_t i = 0; i < drag_data_.size(); ++i) {
    views[i] = drag_data_[i].attached_view.get();
  }

  attached_context_->LayoutDraggedViewsAt(
      std::move(views), source_view_drag_data()->attached_view, point_in_screen,
      initial_move_);

  if (old_split_view_browser_data) {
    auto* new_browser = BrowserView::GetBrowserViewForNativeWindow(
                            GetAttachedBrowserWidget()->GetNativeWindow())
                            ->browser();
    old_split_view_browser_data->TabsAttachedToNewBrowser(new_browser);
  }
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

  if (is_showing_vertical_tabs_) {
    bounds.set_size(widget->GetTopLevelWidget()->GetRestoredBounds().size());
  }

  return bounds;
}

gfx::Rect TabDragController::CalculateDraggedBrowserBounds(
    TabDragContext* source,
    const gfx::Point& point_in_screen,
    std::vector<gfx::Rect>* drag_bounds) {
  // This method is called when creating new browser by detaching tabs and
  // when dragging all tabs in maximized window.
  auto bounds = TabDragControllerChromium::CalculateDraggedBrowserBounds(
      source, point_in_screen, drag_bounds);
  if (is_showing_vertical_tabs_) {
    // Revert back coordinate adjustment done by Chromium impl.
    bounds.set_origin(point_in_screen);

    // Adjust coordinate so that dragged tabs are under cursor.
    DCHECK(!drag_bounds->empty());
    bounds.Offset(-(mouse_offset_.OffsetFromOrigin()));
    bounds.Offset({-drag_bounds->front().x(), 0});
    bounds.Offset({-GetXCoordinateAdjustmentForMultiSelectedTabs(
                       attached_views_, source_view_index_),
                   0});

    auto* browser_view = static_cast<BraveBrowserView*>(
        BrowserView::GetBrowserViewForNativeWindow(
            GetAttachedBrowserWidget()->GetNativeWindow()));
    DCHECK(browser_view);

    auto* widget_delegate_view =
        browser_view->vertical_tab_strip_widget_delegate_view();
    DCHECK(widget_delegate_view);

    bounds.Offset(GetVerticalTabStripWidgetOffset());
    bounds.Offset(-widget_delegate_view->vertical_tab_strip_region_view()
                       ->GetOffsetForDraggedTab());
    bounds.set_size(browser_view->GetRestoredBounds().size());
  }

  return bounds;
}

[[nodiscard]] TabDragController::Liveness TabDragController::ContinueDragging(
    const gfx::Point& point_in_screen) {
  auto* browser_widget = GetAttachedBrowserWidget();
  auto* browser = BrowserView::GetBrowserViewForNativeWindow(
                      browser_widget->GetNativeWindow())
                      ->browser();
  SplitViewBrowserData* split_view_browser_data =
      SplitViewBrowserData::FromBrowser(browser);
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

void TabDragController::InitDragData(TabSlotView* view,
                                     TabDragData* drag_data) {
  // This seems to be a bug from upstream. If the `view` is a group header,
  // there can't be contents or pinned state which are bound to this `view`.
  if (view->GetTabSlotViewType() == TabSlotView::ViewType::kTabGroupHeader) {
    std::optional<tab_groups::TabGroupId> tab_group_id = view->group();
    DCHECK(tab_group_id.has_value());
    drag_data->tab_group_data = TabDragData::TabGroupData{
        tab_group_id.value(), *source_context_->GetTabStripModel()
                                   ->group_model()
                                   ->GetTabGroup(tab_group_id.value())
                                   ->visual_data()};
    return;
  }

  TabDragControllerChromium::InitDragData(view, drag_data);
}
