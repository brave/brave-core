/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_DRAGGING_TAB_DRAG_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_DRAGGING_TAB_DRAG_CONTROLLER_H_

#include "chrome/browser/ui/views/tabs/dragging/tab_drag_context.h"

#define CompleteDrag                   \
  CompleteDrag_Unused();               \
  friend class BraveTabDragController; \
  void CompleteDrag

#define DetachAndAttachToNewContext virtual DetachAndAttachToNewContext
#define StartDraggingTabsSession virtual StartDraggingTabsSession

#include <chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h>  // IWYU pragma: export

#undef StartDraggingTabsSession
#undef DetachAndAttachToNewContext
#undef CompleteDrag

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_DRAGGING_TAB_DRAG_CONTROLLER_H_
