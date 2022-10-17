/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_drag_controller.h"

#define TabDragController TabDragControllerChromium
#include "src/chrome/browser/ui/views/tabs/tab_drag_controller.cc"
#undef TabDragController

#include "brave/browser/ui/views/tabs/features.h"

TabDragController::~TabDragController() = default;

// In order to use inner class defined in tab_drag_controller.cc, this function
// is defined here.
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
  if (!tabs::features::ShouldShowVerticalTabs())
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
  if (!tabs::features::ShouldShowVerticalTabs())
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
  if (tabs::features::ShouldShowVerticalTabs()) {
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
  if (group_id.has_value() || !tabs::features::ShouldShowVerticalTabs())
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
