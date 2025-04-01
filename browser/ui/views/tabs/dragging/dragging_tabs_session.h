/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_DRAGGING_DRAGGING_TABS_SESSION_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_DRAGGING_DRAGGING_TABS_SESSION_H_

#include "chrome/browser/ui/views/tabs/dragging/dragging_tabs_session.h"

class DraggingTabsSession : public DraggingTabsSessionChromium {
 public:
  explicit DraggingTabsSession(DragSessionData drag_data,
                               TabDragContext* attached_context,
                               int mouse_offset,
                               bool initial_move,
                               gfx::Point point_in_screen);
  ~DraggingTabsSession() override;

  void set_mouse_y_offset(int offset) { mouse_y_offset_ = offset; }
  void set_is_showing_vertical_tabs(bool show) {
    is_showing_vertical_tabs_ = show;
  }

  // DraggingTabSessionChromium:
  gfx::Point GetAttachedDragPoint(gfx::Point point_in_screen) override;
  void MoveAttached(gfx::Point point_in_screen) override;

 private:
  int mouse_y_offset_ = 0;
  bool is_showing_vertical_tabs_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_DRAGGING_DRAGGING_TABS_SESSION_H_
