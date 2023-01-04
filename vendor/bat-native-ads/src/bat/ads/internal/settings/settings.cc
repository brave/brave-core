/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/settings/settings.h"

#include <cstdint>

#include "base/cxx17_backports.h"
#include "bat/ads/internal/ads/serving/serving_features.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "brave/components/brave_ads/common/constants.h"
#include "brave/components/brave_ads/common/pref_names.h"

namespace ads::settings {

int GetMaximumNotificationAdsPerHour() {
  int64_t ads_per_hour = AdsClientHelper::GetInstance()->GetInt64Pref(
      prefs::kMaximumNotificationAdsPerHour);

  if (ads_per_hour == -1) {
    ads_per_hour =
        static_cast<int64_t>(features::GetDefaultNotificationAdsPerHour());
  }

  const int64_t clamped_ads_per_hour = base::clamp(
      ads_per_hour, static_cast<int64_t>(kMinimumNotificationAdsPerHour),
      static_cast<int64_t>(kMaximumNotificationAdsPerHour));

  return static_cast<int>(clamped_ads_per_hour);
}

}  // namespace ads::settings
