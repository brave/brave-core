/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/notification_ads/creative_notification_ads_database_util.h"

#include "base/functional/bind.h"
#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ads_database_table.h"

namespace ads::database {

void DeleteCreativeNotificationAds() {
  const table::CreativeNotificationAds database_table;
  database_table.Delete(base::BindOnce([](const bool success) {
    if (!success) {
      BLOG(0, "Failed to delete creative notification ads");
      return;
    }

    BLOG(3, "Successfully deleted creative notification ads");
  }));
}

void SaveCreativeNotificationAds(
    const CreativeNotificationAdList& creative_ads) {
  table::CreativeNotificationAds database_table;
  database_table.Save(creative_ads, base::BindOnce([](const bool success) {
                        if (!success) {
                          BLOG(0, "Failed to save creative notification ads");
                          return;
                        }

                        BLOG(3, "Successfully saved creative notification ads");
                      }));
}

}  // namespace ads::database
