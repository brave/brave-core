// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_ANDROID_AI_CHAT_BRAVE_LEO_SETTINGS_LAUNCHER_HELPER_H_
#define BRAVE_BROWSER_UI_ANDROID_AI_CHAT_BRAVE_LEO_SETTINGS_LAUNCHER_HELPER_H_

#include <string>

namespace content {
class WebContents;
}

namespace ai_chat {

// Opens the Brave Leo settings page.
void ShowBraveLeoSettings(content::WebContents* web_contents);
// Opens an OS subscription dialog.
void GoPremium(content::WebContents* web_contents);
// Opens Manage Subscription in case of IAP of account.brave.com
void ManagePremium(content::WebContents* web_contents);
// Opens any other URL
void OpenURL(const std::string& url);
// Handles voice recognition for a prompt
void HandleVoiceRecognition(content::WebContents* web_contents,
                            const std::string& conversation_uuid);
// Closes Leo chat window
void CloseActivity(content::WebContents* web_contents);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_ANDROID_AI_CHAT_BRAVE_LEO_SETTINGS_LAUNCHER_HELPER_H_
