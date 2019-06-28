/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/version_info.h"
#include "brave/components/brave_ads/browser/ad_notification.h"

#include "base/strings/utf_string_conversions.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/public/cpp/notifier_id.h"

namespace brave_ads {

namespace {

const char kNotifierId[] = "service.ads_service";

}  // namespace

// static
std::unique_ptr<message_center::Notification> CreateAdNotification(
    const ads::NotificationInfo& notification_info) {
  message_center::RichNotificationData notification_data;

  base::string16 advertiser;
  if (base::IsStringUTF8(notification_info.advertiser)) {
    base::UTF8ToUTF16(notification_info.advertiser.c_str(),
                      notification_info.advertiser.length(), &advertiser);
  }

  base::string16 text;
  if (base::IsStringUTF8(notification_info.text)) {
    base::UTF8ToUTF16(notification_info.text.c_str(),
                      notification_info.text.length(), &text);
  }

  // hack to prevent origin from showing in the notification
  // since we're using that to get the notification_id to OpenSettings
  notification_data.context_message = base::ASCIIToUTF16(" ");
  auto notification = std::make_unique<message_center::Notification>(
      message_center::NOTIFICATION_TYPE_SIMPLE,
      notification_info.id,
      advertiser,
      text,
      gfx::Image(),
      base::string16(),
      GURL("chrome://brave_ads/?" + notification_info.id),
      message_center::NotifierId(message_center::NotifierType::SYSTEM_COMPONENT,
          kNotifierId),
      notification_data,
      nullptr);

#if !defined(OS_MACOSX) || defined(OFFICIAL_BUILD)
  // set_never_timeout uses an XPC service which requires signing
  // so for now we don't set this for macos dev builds
  notification->set_never_timeout(true);
#endif

  return notification;
}

}  // namespace brave_ads
