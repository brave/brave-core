/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_AI_CHAT_UTILS_H_
#define BRAVE_BROWSER_UI_AI_CHAT_UTILS_H_

namespace content {
class WebContents;
}

namespace ai_chat {

void OpenAIChatForTab(content::WebContents* web_contents);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_AI_CHAT_UTILS_H_
