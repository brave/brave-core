/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_DRAGGING_DRAGGING_TABS_SESSION_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_DRAGGING_DRAGGING_TABS_SESSION_H_

// In order to replace DraggingTabsSession with ours easily, rename upstream's
// implementation. Ours is in
// brave/browser/ui/views/tabs/dragging_tabs_session.h and the file will be
// included at the end of this file.
class DraggingTabsSession;
using DraggingTabsSessionBrave = DraggingTabsSession;

#define DraggingTabsSession DraggingTabsSessionChromium

#define GetAttachedDragPoint       \
  Unused_GetAttachedDragPoint() {  \
    return {};                     \
  }                                \
  friend DraggingTabsSessionBrave; \
  virtual gfx::Point GetAttachedDragPoint

#include "src/chrome/browser/ui/views/tabs/dragging/dragging_tabs_session.h"  // IWYU pragma: export

#undef GetAttachedDragPoint
#undef DraggingTabsSession

#include "brave/browser/ui/views/tabs/dragging/dragging_tabs_session.h"

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_DRAGGING_DRAGGING_TABS_SESSION_H_
