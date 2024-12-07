/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_util.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/notification_ads/creative_notification_ads_database_table.h"

namespace brave_ads::database {

void DeleteCreativeNotificationAds() {
  const table::CreativeNotificationAds database_table;
  database_table.Delete(base::BindOnce([](bool success) {
    if (!success) {
      BLOG(0, "Failed to delete creative notification ads");
    }
  }));
}

void SaveCreativeNotificationAds(
    const CreativeNotificationAdList& creative_ads) {
  table::CreativeNotificationAds database_table;
  database_table.Save(creative_ads, base::BindOnce([](bool success) {
                        if (!success) {
                          return BLOG(
                              0, "Failed to save creative notification ads");
                        }

                        BLOG(3, "Successfully saved creative notification ads");
                      }));
}

}  // namespace brave_ads::database
