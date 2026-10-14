// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_WIDGET_GESTURE_CONTEXT_MENU_UTIL_H_
#define BRAVE_BROWSER_UI_GESTURES_WIDGET_GESTURE_CONTEXT_MENU_UTIL_H_

#include "base/functional/callback.h"

namespace content {
class WebContents;
}  // namespace content

namespace gestures {

// Shared by every platform's context menu override. Returns true if the
// menu should stay hidden now, either suppressed or deferred until a draw
// resolves.
bool ConsumeSuppressContextMenuForWebContents(
    content::WebContents* web_contents,
    base::OnceClosure show_context_menu);

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_WIDGET_GESTURE_CONTEXT_MENU_UTIL_H_
