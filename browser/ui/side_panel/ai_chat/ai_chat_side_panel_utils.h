// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_UTILS_H_
#define BRAVE_BROWSER_UI_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_UTILS_H_

#include "content/public/browser/web_contents.h"

class Browser;

namespace ai_chat {

// Move this function to browser_finder.
// https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/side_panel/companion/companion_side_panel_controller_utils.h;l=19;drc=e87fd2634a1140a87d59c7aa72668d16e4c102c2
Browser* GetBrowserForWebContents(content::WebContents* web_contents);
void ClosePanel(content::WebContents* web_contents);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_UTILS_H_
