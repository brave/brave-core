/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/notifications/legacy_notification_migration.h"

#include <string>

#include "base/bind.h"
#include "base/containers/circular_deque.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_manager_values_util.h"
#include "bat/ads/internal/legacy_migration/notifications/legacy_notification_json_reader.h"
#include "bat/ads/notification_ad_info.h"
#include "bat/ads/pref_names.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace notifications {

namespace {

constexpr char kNotificationStateFilename[] = "notifications.json";

bool HasMigrated() {
  return AdsClientHelper::GetInstance()->GetBooleanPref(
      prefs::kHasMigratedNotificationState);
}

void FailedToMigrate(InitializeCallback callback) {
  callback(/*success*/ false);
}

void SuccessfullyMigrated(InitializeCallback callback) {
  AdsClientHelper::GetInstance()->SetBooleanPref(
      prefs::kHasMigratedNotificationState, true);
  callback(/*success*/ true);
}

}  // namespace

void Migrate(InitializeCallback callback) {
  if (HasMigrated()) {
    callback(/*success*/ true);
    return;
  }

  BLOG(3, "Loading notification state");

  AdsClientHelper::GetInstance()->Load(
      kNotificationStateFilename,
      base::BindOnce(
          [](InitializeCallback callback, const bool success,
             const std::string& json) {
            if (!success) {
              // Notification state does not exist
              SuccessfullyMigrated(callback);
              return;
            }

            const absl::optional<base::circular_deque<NotificationAdInfo>> ads =
                JSONReader::ReadNotificationAds(json);
            if (!ads) {
              BLOG(0, "Failed to load notification state");
              FailedToMigrate(callback);
              return;
            }

            BLOG(3, "Successfully loaded notification state");

            BLOG(1, "Migrating notification state");

            AdsClientHelper::GetInstance()->SetListPref(
                prefs::kNotificationAds, NotificationAdsToValue(*ads));

            BLOG(3, "Successfully migrated notification state");
            SuccessfullyMigrated(callback);
          },
          callback));
}

}  // namespace notifications
}  // namespace ads
