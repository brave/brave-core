/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/ad_notifications/creative_ad_notifications_database_util.h"

#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/creatives/ad_notifications/creative_ad_notification_info.h"
#include "bat/ads/internal/creatives/ad_notifications/creative_ad_notifications_database_table.h"

namespace ads {
namespace database {

void DeleteCreativeAdNotifications() {
  table::CreativeAdNotifications database_table;
  database_table.Delete([](const bool success) {
    if (!success) {
      BLOG(0, "Failed to delete creative ad notifications");
      return;
    }

    BLOG(3, "Successfully deleted creative ad notifications");
  });
}

void SaveCreativeAdNotifications(
    const CreativeAdNotificationList& creative_ads) {
  table::CreativeAdNotifications database_table;
  database_table.Save(creative_ads, [](const bool success) {
    if (!success) {
      BLOG(0, "Failed to save creative ad notifications");
      return;
    }

    BLOG(3, "Successfully saved creative ad notifications");
  });
}

}  // namespace database
}  // namespace ads
