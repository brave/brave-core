// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/ai_chat_notification_helper.h"

#include <memory>
#include <optional>
#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/strings/strcat.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/notifications/notification_handler.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_delegate.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/public/cpp/notifier_id.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

constexpr char kAIChatNotificationIdPrefix[] = "ai_chat.";
constexpr char kAIChatNotifierId[] = "brave.ai_chat";

// TODO(https://github.com/brave/brave-browser/issues/60355): Navigate to the
// conversation on click.
class AIChatNotificationDelegate : public message_center::NotificationDelegate {
 public:
  AIChatNotificationDelegate() = default;

  void Click(const std::optional<int>& button_index,
             const std::optional<std::u16string>& reply) override {}

 private:
  ~AIChatNotificationDelegate() override = default;
};

std::string GetAIChatNotificationId(const std::string& conversation_uuid) {
  return base::StrCat({kAIChatNotificationIdPrefix, conversation_uuid});
}

}  // namespace

void ShowAIChatCompletionNotification(
    Profile* profile,
    const std::string& conversation_uuid,
    const std::u16string& conversation_title) {
  if (!profile) {
    return;
  }
  auto* display_service =
      NotificationDisplayServiceFactory::GetForProfile(profile);
  if (!display_service) {
    return;
  }

  const std::string notification_id =
      GetAIChatNotificationId(conversation_uuid);

  const std::u16string title =
      l10n_util::GetStringUTF16(IDS_AI_CHAT_NOTIFICATION_TITLE);
  std::u16string message = conversation_title;
  if (message.empty()) {
    message = l10n_util::GetStringUTF16(IDS_AI_CHAT_NOTIFICATION_BODY_FALLBACK);
  }

  message_center::RichNotificationData data;

  auto notification = std::make_unique<message_center::Notification>(
      message_center::NOTIFICATION_TYPE_SIMPLE, notification_id, title, message,
      ui::ImageModel(), std::u16string(), GURL(),
      message_center::NotifierId(message_center::NotifierType::SYSTEM_COMPONENT,
                                 kAIChatNotifierId),
      data, base::MakeRefCounted<AIChatNotificationDelegate>());

  display_service->Display(NotificationHandler::Type::TRANSIENT, *notification,
                           nullptr);
}

void CloseAIChatCompletionNotification(Profile* profile,
                                       const std::string& conversation_uuid) {
  if (!profile) {
    return;
  }
  auto* display_service =
      NotificationDisplayServiceFactory::GetForProfile(profile);
  if (!display_service) {
    return;
  }
  display_service->Close(NotificationHandler::Type::TRANSIENT,
                         GetAIChatNotificationId(conversation_uuid));
}

}  // namespace ai_chat
