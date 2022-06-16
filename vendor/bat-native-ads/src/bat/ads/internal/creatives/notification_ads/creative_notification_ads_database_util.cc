/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/notification_ads/creative_notification_ads_database_util.h"

#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ads_database_table.h"

namespace ads {
namespace database {

void DeleteCreativeNotificationAds() {
  table::CreativeNotificationAds database_table;
  database_table.Delete([](const bool success) {
    if (!success) {
      BLOG(0, "Failed to delete creative notification ads");
      return;
    }

    BLOG(3, "Successfully deleted creative notification ads");
  });
}

void SaveCreativeNotificationAds(
    const CreativeNotificationAdList& creative_ads) {
  table::CreativeNotificationAds database_table;
  database_table.Save(creative_ads, [](const bool success) {
    if (!success) {
      BLOG(0, "Failed to save creative notification ads");
      return;
    }

    BLOG(3, "Successfully saved creative notification ads");
  });
}

}  // namespace database
}  // namespace ads
