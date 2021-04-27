/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ad_notification.h"

#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "bat/ads/ad_notification_info.h"
#include "brave/ui/brave_ads/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/public/cpp/notifier_id.h"

namespace brave_ads {

namespace {

const char kNotifierId[] = "service.ads_service";

}  // namespace

// static
std::unique_ptr<brave_ads::Notification> CreateAdNotification(
    const ads::AdNotificationInfo& info) {
  brave_ads::RichNotificationData notification_data;

  std::u16string title;
  if (base::IsStringUTF8(info.title)) {
    base::UTF8ToUTF16(info.title.c_str(), info.title.length(), &title);
  }

  std::u16string body;
  if (base::IsStringUTF8(info.body)) {
    base::UTF8ToUTF16(info.body.c_str(), info.body.length(), &body);
  }

  // hack to prevent origin from showing in the notification
  // since we're using that to get the notification_id to OpenSettings
  notification_data.context_message = u" ";
  auto notification = std::make_unique<brave_ads::Notification>(
      info.uuid, title, body, std::u16string(), GURL(info.uuid),
      notification_data, nullptr);

#if !defined(OS_MAC) || defined(OFFICIAL_BUILD)
  // set_never_timeout uses an XPC service which requires signing
  // so for now we don't set this for macos dev builds
  notification->set_never_timeout(true);
#endif

  return notification;
}

// static
std::unique_ptr<message_center::Notification> CreateMessageCenterNotification(
    const ads::AdNotificationInfo& info) {
  message_center::RichNotificationData notification_data;

  std::u16string title;
  if (base::IsStringUTF8(info.title)) {
    base::UTF8ToUTF16(info.title.c_str(), info.title.length(), &title);
  }

  std::u16string body;
  if (base::IsStringUTF8(info.body)) {
    base::UTF8ToUTF16(info.body.c_str(), info.body.length(), &body);
  }

  // hack to prevent origin from showing in the notification
  // since we're using that to get the notification_id to OpenSettings
  notification_data.context_message = u" ";
  auto notification = std::make_unique<message_center::Notification>(
      message_center::NOTIFICATION_TYPE_SIMPLE, info.uuid, title, body,
      gfx::Image(), std::u16string(), GURL(kBraveAdsUrlPrefix + info.uuid),
      message_center::NotifierId(message_center::NotifierType::SYSTEM_COMPONENT,
                                 kNotifierId),
      notification_data, nullptr);

#if !defined(OS_MAC) || defined(OFFICIAL_BUILD)
  // set_never_timeout uses an XPC service which requires signing
  // so for now we don't set this for macos dev builds
  notification->set_never_timeout(true);
#endif

  return notification;
}

}  // namespace brave_ads
