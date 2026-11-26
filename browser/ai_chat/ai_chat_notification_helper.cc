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
class AIChatNotificationClickDelegate
    : public message_center::NotificationDelegate {
 public:
  AIChatNotificationClickDelegate() = default;

  void Click(const std::optional<int>& button_index,
             const std::optional<std::u16string>& reply) override {}

 private:
  ~AIChatNotificationClickDelegate() override = default;
};

std::string GetAIChatNotificationId(const std::string& conversation_uuid) {
  return base::StrCat({kAIChatNotificationIdPrefix, conversation_uuid});
}

}  // namespace

AIChatNotificationHelper::AIChatNotificationHelper(Profile* profile)
    : profile_(profile) {}

AIChatNotificationHelper::~AIChatNotificationHelper() = default;

void AIChatNotificationHelper::ShowCompletionNotification(
    const std::string& conversation_uuid,
    const std::u16string& conversation_title) {
  auto* display_service =
      NotificationDisplayServiceFactory::GetForProfile(profile_);
  if (!display_service) {
    return;
  }

  const std::string notification_id =
      GetAIChatNotificationId(conversation_uuid);

  const std::u16string title =
      conversation_title.empty()
          ? l10n_util::GetStringUTF16(IDS_CHAT_UI_TITLE)
          : l10n_util::GetStringFUTF16(IDS_AI_CHAT_NOTIFICATION_TITLE,
                                       conversation_title);
  const std::u16string message =
      l10n_util::GetStringUTF16(IDS_AI_CHAT_NOTIFICATION_BODY);

  message_center::RichNotificationData data;

  auto notification = std::make_unique<message_center::Notification>(
      message_center::NOTIFICATION_TYPE_SIMPLE, notification_id, title, message,
      ui::ImageModel(), std::u16string(), GURL(),
      message_center::NotifierId(message_center::NotifierType::SYSTEM_COMPONENT,
                                 kAIChatNotifierId),
      data, base::MakeRefCounted<AIChatNotificationClickDelegate>());

  display_service->Display(NotificationHandler::Type::TRANSIENT, *notification,
                           nullptr);
}

void AIChatNotificationHelper::CloseCompletionNotification(
    const std::string& conversation_uuid) {
  auto* display_service =
      NotificationDisplayServiceFactory::GetForProfile(profile_);
  if (!display_service) {
    return;
  }
  display_service->Close(NotificationHandler::Type::TRANSIENT,
                         GetAIChatNotificationId(conversation_uuid));
}

}  // namespace ai_chat
