/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"

namespace brave_ads {

void DisableBraveRewardsForTesting() {
  SetBooleanPref(brave_rewards::prefs::kEnabled, false);
}

void OptOutOfBraveNewsAdsForTesting() {
  SetBooleanPref(brave_news::prefs::kBraveNewsOptedIn, false);
  SetBooleanPref(brave_news::prefs::kNewTabPageShowToday, false);
}

void OptOutOfNewTabPageAdsForTesting() {
  SetBooleanPref(ntp_background_images::prefs::kNewTabPageShowBackgroundImage,
                 false);
  SetBooleanPref(ntp_background_images::prefs::
                     kNewTabPageShowSponsoredImagesBackgroundImage,
                 false);
}

void OptOutOfNotificationAdsForTesting() {
  SetBooleanPref(prefs::kOptedInToNotificationAds, false);
}

void SetMaximumNotificationAdsPerHourForTesting(const int max_ads_per_hour) {
  SetInt64Pref(prefs::kMaximumNotificationAdsPerHour, max_ads_per_hour);
}

}  // namespace brave_ads
