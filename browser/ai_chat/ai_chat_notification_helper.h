// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_NOTIFICATION_HELPER_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_NOTIFICATION_HELPER_H_

#include <string>

class Profile;

namespace ai_chat {

// Shows a notification for a completed background generation.
void ShowAIChatCompletionNotification(Profile* profile,
                                      const std::string& conversation_uuid,
                                      const std::u16string& conversation_title);

// Closes the notification for |conversation_uuid|.
void CloseAIChatCompletionNotification(Profile* profile,
                                       const std::string& conversation_uuid);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_NOTIFICATION_HELPER_H_
