/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_DRAG_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_DRAG_CONTROLLER_H_

class TabDragController;
using TabDragControllerBrave = TabDragController;

#define TabDragController TabDragControllerChromium

#define GetAttachedDragPoint                   \
  Unused_GetAttachedDragPoint() { return {}; } \
  virtual gfx::Point GetAttachedDragPoint

#define MoveAttached             \
  Unused_MoveUattached() {}      \
  friend TabDragControllerBrave; \
  virtual void MoveAttached

#include "src/chrome/browser/ui/views/tabs/tab_drag_controller.h"
#undef TabDragController
#undef MoveAttached
#undef GetAttachedDragPoint

class TabDragController : public TabDragControllerChromium {
 public:
  using TabDragControllerChromium::TabDragControllerChromium;
  ~TabDragController() override;

  // Making this virtual method is really painful as "Init" is too common name.
  // So, just hide Chromium's Init and make clients use this version
  void Init(TabDragContext* source_context,
            TabSlotView* source_view,
            const std::vector<TabSlotView*>& dragging_views,
            const gfx::Point& mouse_offset,
            int source_view_offset,
            ui::ListSelectionModel initial_selection_model,
            EventSource event_source);

  // TabDragControllerChromium:
  gfx::Point GetAttachedDragPoint(const gfx::Point& point_in_screen) override;
  void MoveAttached(const gfx::Point& point_in_screen,
                    bool just_attached) override;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_DRAG_CONTROLLER_H_
