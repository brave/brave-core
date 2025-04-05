/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/dragging/dragging_tabs_session.h"

#include "chrome/browser/ui/views/tabs/dragging/drag_session_data.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_context.h"
#include "ui/gfx/geometry/point.h"

DraggingTabsSession::DraggingTabsSession(DragSessionData drag_data,
                                         TabDragContext* attached_context,
                                         int mouse_offset,
                                         bool initial_move,
                                         gfx::Point start_point_in_screen)
    : DraggingTabsSessionChromium(drag_data,
                                  attached_context,
                                  mouse_offset,
                                  initial_move,
                                  start_point_in_screen) {}

DraggingTabsSession::~DraggingTabsSession() {}

gfx::Point DraggingTabsSession::GetAttachedDragPoint(
    gfx::Point point_in_screen) {
  if (!is_showing_vertical_tabs_) {
    return DraggingTabsSessionChromium::GetAttachedDragPoint(point_in_screen);
  }

  DCHECK(attached_context_);  // The tab must be attached.
  gfx::Point tab_loc(point_in_screen);
  views::View::ConvertPointFromScreen(attached_context_, &tab_loc);
  const int x = drag_data_.tab_drag_data_.front().pinned
                    ? tab_loc.x() - mouse_offset_
                    : 0;
  const int y = tab_loc.y() - mouse_y_offset_;
  return {x, y};
}

void DraggingTabsSession::MoveAttached(gfx::Point point_in_screen) {
  DraggingTabsSessionChromium::MoveAttached(point_in_screen);
  if (!is_showing_vertical_tabs_) {
    return;
  }

  // Unlike upstream, We always update coordinate, as we use y coordinate. Since
  // we don't have threshold there's no any harm for this.
  views::View::ConvertPointFromScreen(attached_context_, &point_in_screen);
  last_move_attached_context_loc_ = point_in_screen.y();
}
