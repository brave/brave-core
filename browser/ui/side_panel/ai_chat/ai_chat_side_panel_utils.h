// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_UTILS_H_
#define BRAVE_BROWSER_UI_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_UTILS_H_

#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

class Browser;
class Profile;

namespace ai_chat {

// Move this function to browser_finder.
// https://source.chromium.org/chromium/chromium/src/+/main:chrome/browser/ui/side_panel/companion/companion_side_panel_controller_utils.h;l=19;drc=e87fd2634a1140a87d59c7aa72668d16e4c102c2
Browser* GetBrowserForWebContents(content::WebContents* web_contents);
void ClosePanel(content::WebContents* web_contents);

// Forward (tab -> side panel): when `ai_chat_web_contents` is a full browser
// tab and the `kAIChatMoveFullPageToSidePanel` feature is enabled, opens
// `link_url` in a tab at AI Chat's position and moves the live AI Chat
// `WebContents` into the side panel. Returns true if the click was handled this
// way (the caller must then NOT open `link_url` itself). Desktop only; the
// definition lives in the toolkit_views translation unit.
bool MaybeMoveFullPageChatToSidePanel(
    content::WebContents* ai_chat_web_contents,
    const GURL& link_url);

// Closes the side panel only if the AI Chat entry is the one currently being
// shown. Used when moving a conversation to a full-page tab, where leaving the
// now-duplicate AI Chat side panel open would be confusing.
void ClosePanelIfChatActive(content::WebContents* web_contents);

// Returns true if the side panel should be global for all tabs in a tab strip,
// or false if it should be per-tab.
bool ShouldSidePanelBeGlobal(Profile* profile);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_SIDE_PANEL_AI_CHAT_AI_CHAT_SIDE_PANEL_UTILS_H_
