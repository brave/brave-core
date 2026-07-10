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
#include "brave/browser/ui/tabs/public/vertical_tab_controller.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_container_view.h"
#include "brave/browser/ui/views/frame/vertical_tabs/vertical_tab_strip_region_view.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_context.h"
#include "chrome/browser/ui/views/tabs/dragging/window_finder.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_group.h"
#include "ui/views/view_utils.h"

BraveTabDragController::BraveTabDragController() = default;

BraveTabDragController::~BraveTabDragController() = default;

BraveTabDragController::Liveness BraveTabDragController::Init(
    TabDragContext* source_context,
    TabSlotView* source_view,
    const std::vector<TabSlotView*>& dragging_views,
    const gfx::Point& offset_from_source_view,
    ui::ListSelectionModel initial_selection_model,
    ui::mojom::DragEventSource event_source) {
  if (TabDragController::Init(source_context, source_view, dragging_views,
                              offset_from_source_view, initial_selection_model,
                              event_source) ==
      BraveTabDragController::BraveTabDragController::Liveness::kDeleted) {
    return BraveTabDragController::BraveTabDragController::Liveness::kDeleted;
  }

  offset_from_first_dragged_view_ = offset_from_source_view;

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

  is_showing_vertical_tabs_ = VerticalTabController::FromBrowser(browser)
                                  ->ShouldShowBraveVerticalTabs();

  return BraveTabDragController::BraveTabDragController::Liveness::kAlive;
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

void BraveTabDragController::DetachAndAttachToNewContext(
    ReleaseCapture release_capture,
    TabDragContext* target_context) {
  if (!is_showing_vertical_tabs_) {
    TabDragController::DetachAndAttachToNewContext(release_capture,
                                                   target_context);
    return;
  }

  auto get_region_view = [this] {
    auto* browser_view =
        BraveBrowserView::From(BrowserView::GetBrowserViewForNativeWindow(
            GetAttachedBrowserWidget()->GetNativeWindow()));
    DCHECK(browser_view);

    auto* container_view = browser_view->vertical_tab_strip_container_view();
    DCHECK(container_view);

    auto* region_view = container_view->vertical_tab_strip_region_view();
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
