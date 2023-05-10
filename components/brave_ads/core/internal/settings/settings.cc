/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/settings/settings.h"

#include "brave/components/brave_ads/common/notification_ad_feature.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"

namespace brave_ads {

int GetMaximumNotificationAdsPerHourSetting() {
  int ads_per_hour =
      static_cast<int>(AdsClientHelper::GetInstance()->GetInt64Pref(
          prefs::kMaximumNotificationAdsPerHour));

  if (ads_per_hour == -1) {
    ads_per_hour = kDefaultNotificationAdsPerHour.Get();
  }

  return ads_per_hour;
}

}  // namespace brave_ads
