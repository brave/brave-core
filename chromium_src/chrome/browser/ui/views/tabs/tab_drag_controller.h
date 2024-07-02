/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_DRAG_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_DRAG_CONTROLLER_H_

// In order to replace TabDragController with ours easily, rename upstream's
// implementation. Ours is in brave/browser/ui/views/tabs/tab_drag_controller.h
// and the file will be included at the end of this file.
class TabDragController;
using TabDragControllerBrave = TabDragController;

#define TabDragController TabDragControllerChromium

#define GetAttachedDragPoint                   \
  Unused_GetAttachedDragPoint() { return {}; } \
  virtual gfx::Point GetAttachedDragPoint

#define InitDragData             \
  Unused_MoveUattached() {}      \
  friend TabDragControllerBrave; \
  virtual void InitDragData
#define GetAttachedBrowserWidget                   \
  GetAttachedBrowserWidget_Unused() { return {}; } \
  virtual views::Widget* GetAttachedBrowserWidget
#define GetLocalProcessWindow virtual GetLocalProcessWindow
#define DetachAndAttachToNewContext virtual DetachAndAttachToNewContext
#define CalculateNonMaximizedDraggedBrowserBounds \
  virtual CalculateNonMaximizedDraggedBrowserBounds
#define CalculateDraggedBrowserBounds virtual CalculateDraggedBrowserBounds
#define ContinueDragging virtual ContinueDragging

#include "src/chrome/browser/ui/views/tabs/tab_drag_controller.h"  // IWYU pragma: export

#undef ContinueDragging
#undef CalculateDraggedBrowserBounds
#undef CalculateNonMaximizedDraggedBrowserBounds
#undef DetachAndAttachToNewContext
#undef GetLocalProcessWindow
#undef GetAttachedBrowserWidget
#undef TabDragController
#undef InitDragData
#undef GetAttachedDragPoint

#include "brave/browser/ui/views/tabs/tab_drag_controller.h"

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_DRAG_CONTROLLER_H_
