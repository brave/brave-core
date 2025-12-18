/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_DRAGGING_TEST_MOCK_TAB_DRAG_CONTEXT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_DRAGGING_TEST_MOCK_TAB_DRAG_CONTEXT_H_

#include "brave/browser/ui/views/tabs/dragging/tab_drag_controller.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_context.h"
#include "chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h"

#define TabDragController TabDragControllerChromium
#include <chrome/browser/ui/views/tabs/dragging/test/mock_tab_drag_context.h>  // IWYU pragma: export
#undef TabDragController

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_DRAGGING_TEST_MOCK_TAB_DRAG_CONTEXT_H_
