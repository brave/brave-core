/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_TABS_DRAGGING_DRAGGING_TABS_SESSION_H_
#define BRAVE_BROWSER_UI_VIEWS_TABS_DRAGGING_DRAGGING_TABS_SESSION_H_

#include <optional>

#include "brave/browser/ui/tabs/tree_tab_drag_hint.h"
#include "chrome/browser/ui/views/tabs/dragging/dragging_tabs_session.h"

class DraggingTabsSession : public DraggingTabsSessionChromium {
 public:
  explicit DraggingTabsSession(
      DragSessionData drag_data,
      TabDragContext& attached_context,
      TabDragPositioningDelegate& drag_position_delegate,
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
  std::optional<tab_groups::TabGroupId> CalculateGroupForDraggedTabs(
      int to_index) override;

 private:
  // Returns true if the nesting level of dragged tabs can be adjusted, i.e.
  // dragging unpinned tabs (not a group header) in tree tab mode.
  bool CanAdjustTreeTabNestingLevel() const;

  // Calculates how the dragged tabs should be nested when they're moved to
  // |to_index|, based on the vertical position of the dragged views relative
  // to the bottom of the tab right above the drop position:
  //   above the threshold - sibling of the tab above
  //   within the threshold - child of the tab above
  //   below the threshold - sibling of the tab below (or a top-level node when
  //                         there's no tab below)
  // Returns std::nullopt when the hint can't be calculated.
  std::optional<TreeTabDragTarget> CalculateTreeTabDragTarget(
      const gfx::Rect& dragged_bounds,
      int to_index);

  int mouse_y_offset_ = 0;
  bool is_showing_vertical_tabs_ = false;

  // The drag target that was applied by the last move.
  std::optional<TreeTabDragTarget> last_tree_tab_drag_target_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_TABS_DRAGGING_DRAGGING_TABS_SESSION_H_
