// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_GESTURE_CONTEXT_MENU_UTIL_H_
#define BRAVE_BROWSER_UI_GESTURES_GESTURE_CONTEXT_MENU_UTIL_H_

#include "base/functional/callback.h"

namespace content {
class WebContents;
}  // namespace content

namespace gestures {

// Looks up `web_contents`'s `GestureController` so every platform's context
// menu override (Views, Mac Cocoa, Mac remote Cocoa) shares one
// implementation rather than duplicating the lookup. Returns true if the
// caller should not show the context menu right now: either it is already
// known to be suppressed (a gesture/rocker just fired), or its outcome is
// still unknown and `show_context_menu` has been deferred via
// `GestureController::DeferContextMenuShow` to run later if the hold turns
// out to be a plain click. Returns false if the caller should show the menu
// immediately.
bool ConsumeSuppressContextMenuForWebContents(
    content::WebContents* web_contents,
    base::OnceClosure show_context_menu);

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_GESTURE_CONTEXT_MENU_UTIL_H_
