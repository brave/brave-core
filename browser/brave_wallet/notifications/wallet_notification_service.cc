/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/notifications/wallet_notification_service.h"

#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "chrome/browser/notifications/notification_display_service.h"
#include "chrome/browser/notifications/notification_display_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/public/cpp/notifier_id.h"

namespace {
int GetStatusTitle(brave_wallet::mojom::TransactionStatus status) {
  switch (status) {
    case brave_wallet::mojom::TransactionStatus::Confirmed:
      return IDS_WALLET_TRANSACTION_STATUS_UPDATE_MESSAGE_TITLE_CONFIRMED;
    case brave_wallet::mojom::TransactionStatus::Error:
      return IDS_WALLET_TRANSACTION_STATUS_UPDATE_MESSAGE_TITLE_ERROR;
    case brave_wallet::mojom::TransactionStatus::Dropped:
      return IDS_WALLET_TRANSACTION_STATUS_UPDATE_MESSAGE_TITLE_DROPPED;
    default:
      break;
  }
  VLOG(1) << "No title for " << int(status) << " transaction status";
  return -1;
}

std::unique_ptr<message_center::Notification> CreateMessageCenterNotification(
    const std::u16string& title,
    const std::u16string& body,
    const std::string& uuid,
    const GURL& link) {
  message_center::RichNotificationData notification_data;
  // hack to prevent origin from showing in the notification
  notification_data.context_message = u" ";
  auto notification = std::make_unique<message_center::Notification>(
      message_center::NOTIFICATION_TYPE_SIMPLE, uuid, title, body,
      ui::ImageModel(), std::u16string(), link,
      message_center::NotifierId(message_center::NotifierType::SYSTEM_COMPONENT,
                                 "service.wallet"),
      notification_data, nullptr);

  return notification;
}

void PushNotification(content::BrowserContext* context,
                      const std::string& uuid,
                      const std::string& from,
                      const std::u16string& title,
                      const std::u16string& body) {
  auto notification = CreateMessageCenterNotification(
      title, body, uuid,
      GURL("brave://wallet/crypto/accounts/" + from + "#" + uuid));
  auto* profile = Profile::FromBrowserContext(context);
  NotificationDisplayServiceFactory::GetForProfile(profile)->Display(
      NotificationHandler::Type::SEND_TAB_TO_SELF, *notification, nullptr);
}

}  // namespace

namespace brave_wallet {

WalletNotificationService::WalletNotificationService(
    content::BrowserContext* context)
    : context_(context) {}

WalletNotificationService::~WalletNotificationService() = default;

bool WalletNotificationService::ShouldDisplayUserNotification(
    mojom::TransactionStatus status) {
  return (status == mojom::TransactionStatus::Confirmed ||
          status == mojom::TransactionStatus::Error ||
          status == mojom::TransactionStatus::Dropped);
}

void WalletNotificationService::DisplayUserNotification(
    mojom::TransactionStatus status,
    const std::string& address,
    const std::string& tx_id) {
  PushNotification(context_, tx_id, address,
                   l10n_util::GetStringUTF16(GetStatusTitle(status)),
                   l10n_util::GetStringFUTF16(
                       IDS_WALLET_TRANSACTION_STATUS_UPDATE_MESSAGE_TEXT,
                       base::UTF8ToUTF16(address)));
}

void WalletNotificationService::OnTransactionStatusChanged(
    mojom::TransactionInfoPtr tx_info) {
  if (ShouldDisplayUserNotification(tx_info->tx_status)) {
    DisplayUserNotification(tx_info->tx_status, tx_info->from_address,
                            tx_info->id);
  }
}

}  // namespace brave_wallet
