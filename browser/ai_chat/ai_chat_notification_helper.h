// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_AI_CHAT_NOTIFICATION_HELPER_H_
#define BRAVE_BROWSER_AI_CHAT_AI_CHAT_NOTIFICATION_HELPER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"

class Profile;

namespace ai_chat {

// Shows background completion notifications through the profile's
// NotificationDisplayService.
class AIChatNotificationHelper : public AIChatService::NotificationDelegate {
 public:
  explicit AIChatNotificationHelper(Profile* profile);
  ~AIChatNotificationHelper() override;
  AIChatNotificationHelper(const AIChatNotificationHelper&) = delete;
  AIChatNotificationHelper& operator=(const AIChatNotificationHelper&) = delete;

  // AIChatService::NotificationDelegate:
  void ShowCompletionNotification(
      const std::string& conversation_uuid,
      const std::u16string& conversation_title) override;
  void CloseCompletionNotification(
      const std::string& conversation_uuid) override;

 private:
  raw_ptr<Profile> profile_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_AI_CHAT_NOTIFICATION_HELPER_H_
