/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/settings/settings.h"

#include <cstdint>

#include "base/cxx17_backports.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ad_serving/ad_serving_features.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/pref_names.h"

namespace ads {
namespace settings {

int GetAdsPerHour() {
  int64_t ads_per_hour =
      AdsClientHelper::Get()->GetInt64Pref(prefs::kAdsPerHour);

  if (ads_per_hour == -1) {
    ads_per_hour =
        static_cast<uint64_t>(features::GetDefaultAdNotificationsPerHour());
  }

  const int64_t clamped_ads_per_hour = base::clamp(
      ads_per_hour, static_cast<int64_t>(kMinimumAdNotificationsPerHour),
      static_cast<int64_t>(kMaximumAdNotificationsPerHour));

  return static_cast<int>(clamped_ads_per_hour);
}

}  // namespace settings
}  // namespace ads
