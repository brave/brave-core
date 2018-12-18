/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ad_notification.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/public/cpp/notifier_id.h"

namespace brave_ads {

namespace {

const char kNotifierIdPrefix[] = "service.ads_service.";
const char kNotifierId[] = "service.ads_service";

}  // namespace

// static
std::unique_ptr<message_center::Notification> CreateAdNotification(
    const ads::NotificationInfo& notification_info,
    std::string* notification_id) {
  *notification_id = kNotifierIdPrefix + notification_info.uuid;
  message_center::RichNotificationData notification_data;
  // hack to prevent origin from showing in the notification
  // since we're using that to get the notification_id to OpenSettings
  notification_data.context_message = base::ASCIIToUTF16(" ");
  auto notification = std::make_unique<message_center::Notification>(
      message_center::NOTIFICATION_TYPE_SIMPLE,
      *notification_id,
      base::ASCIIToUTF16(notification_info.advertiser),
      base::ASCIIToUTF16(notification_info.text),
      gfx::Image(),
      base::string16(),
      GURL("chrome://brave_ads/?" + *notification_id),
      message_center::NotifierId(message_center::NotifierType::SYSTEM_COMPONENT,
                                 kNotifierId),
      notification_data,
      nullptr);

  return notification;
}

}  // namespace brave_ads
