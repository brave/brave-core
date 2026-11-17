// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_NOTIFICATION_DELEGATE_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_NOTIFICATION_DELEGATE_H_

#include <string>

namespace ai_chat {

// Lets the embedder tell the user when a response finishes in the background.
class AIChatNotificationDelegate {
 public:
  virtual ~AIChatNotificationDelegate() = default;

  // A response in |conversation_uuid| finished while no client was connected
  // to it. |conversation_title| is empty if the conversation has none.
  virtual void ShowCompletionNotification(
      const std::string& conversation_uuid,
      const std::u16string& conversation_title) = 0;

  // The conversation was viewed or deleted, or is no longer kept, so its
  // notification is out of date.
  virtual void CloseCompletionNotification(
      const std::string& conversation_uuid) = 0;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_AI_CHAT_NOTIFICATION_DELEGATE_H_
