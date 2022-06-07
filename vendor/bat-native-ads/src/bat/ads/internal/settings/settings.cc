/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/settings/settings.h"

#include "base/cxx17_backports.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/serving/serving_features.h"
#include "bat/ads/pref_names.h"

namespace ads {
namespace settings {

int GetAdsPerHour() {
  int ads_per_hour = AdsClientHelper::Get()->GetIntegerPref(prefs::kAdsPerHour);
  if (ads_per_hour == -1) {
    ads_per_hour = features::GetDefaultNotificationAdsPerHour();
  }

  return base::clamp(ads_per_hour, kMinimumNotificationAdsPerHour,
                     kMaximumNotificationAdsPerHour);
}

}  // namespace settings
}  // namespace ads
