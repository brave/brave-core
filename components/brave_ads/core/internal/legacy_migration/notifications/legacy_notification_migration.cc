/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/legacy_migration/notifications/legacy_notification_migration.h"

#include <string>
#include <utility>

#include "base/containers/circular_deque.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/notifications/legacy_notification_json_reader.h"
#include "brave/components/brave_ads/core/notification_ad_info.h"
#include "brave/components/brave_ads/core/notification_ad_value_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::notifications {

namespace {

constexpr char kNotificationStateFilename[] = "notifications.json";

bool HasMigrated() {
  return AdsClientHelper::GetInstance()->GetBooleanPref(
      prefs::kHasMigratedNotificationState);
}

void FailedToMigrate(InitializeCallback callback) {
  std::move(callback).Run(/*success*/ false);
}

void SuccessfullyMigrated(InitializeCallback callback) {
  AdsClientHelper::GetInstance()->SetBooleanPref(
      prefs::kHasMigratedNotificationState, true);
  std::move(callback).Run(/*success*/ true);
}

}  // namespace

void Migrate(InitializeCallback callback) {
  if (HasMigrated()) {
    return std::move(callback).Run(/*success*/ true);
  }

  AdsClientHelper::GetInstance()->Load(
      kNotificationStateFilename,
      base::BindOnce(
          [](InitializeCallback callback,
             const absl::optional<std::string>& json) {
            if (!json) {
              // Notification state does not exist
              return SuccessfullyMigrated(std::move(callback));
            }

            const absl::optional<base::circular_deque<NotificationAdInfo>> ads =
                json::reader::ReadNotificationAds(*json);
            if (!ads) {
              BLOG(0, "Failed to load notification state");
              return FailedToMigrate(std::move(callback));
            }

            BLOG(1, "Migrating notification state");

            AdsClientHelper::GetInstance()->SetListPref(
                prefs::kNotificationAds, NotificationAdsToValue(*ads));

            BLOG(3, "Successfully migrated notification state");
            SuccessfullyMigrated(std::move(callback));
          },
          std::move(callback)));
}

}  // namespace brave_ads::notifications
