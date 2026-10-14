// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_INPUT_GESTURE_INPUT_REGISTRAR_UTIL_H_
#define BRAVE_BROWSER_UI_GESTURES_INPUT_GESTURE_INPUT_REGISTRAR_UTIL_H_

#include <vector>

namespace content {
class RenderWidgetHost;
class WebContents;
}  // namespace content

namespace gestures {

// Every render widget host reachable from `web_contents`'s current primary
// page, deduped. Empty if `web_contents` is null.
std::vector<content::RenderWidgetHost*> CollectRenderWidgetHosts(
    content::WebContents* web_contents);

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_INPUT_GESTURE_INPUT_REGISTRAR_UTIL_H_
