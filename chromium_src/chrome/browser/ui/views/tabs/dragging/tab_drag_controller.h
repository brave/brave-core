/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_DRAGGING_TAB_DRAG_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_DRAGGING_TAB_DRAG_CONTROLLER_H_

// In order to replace TabDragController with ours easily, rename upstream's
// implementation. Ours is in brave/browser/ui/views/tabs/tab_drag_controller.h
// and the file will be included at the end of this file.
class TabDragController;
using TabDragControllerBrave = TabDragController;

#define TabDragController TabDragControllerChromium

#define CompleteDrag             \
  CompleteDrag_Unused();         \
  friend TabDragControllerBrave; \
  void CompleteDrag

#define GetAttachedBrowserWidget      \
  GetAttachedBrowserWidget_Unused() { \
    return {};                        \
  }                                   \
  virtual views::Widget* GetAttachedBrowserWidget

#define CalculateWindowDragOffset      \
  CalculateWindowDragOffset_Unused() { \
    return {};                         \
  }                                    \
  virtual gfx::Vector2d CalculateWindowDragOffset

#define GetLocalProcessWindow virtual GetLocalProcessWindow
#define DetachAndAttachToNewContext virtual DetachAndAttachToNewContext
#define ContinueDragging virtual ContinueDragging
#define StartDraggingTabsSession virtual StartDraggingTabsSession

#include "src/chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h"  // IWYU pragma: export

#undef StartDraggingTabsSession
#undef ContinueDragging
#undef DetachAndAttachToNewContext
#undef GetLocalProcessWindow
#undef CalculateWindowDragOffset
#undef GetAttachedBrowserWidget
#undef TabDragController
#undef CompleteDrag

#include "brave/browser/ui/views/tabs/dragging/tab_drag_controller.h"

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_DRAGGING_TAB_DRAG_CONTROLLER_H_
