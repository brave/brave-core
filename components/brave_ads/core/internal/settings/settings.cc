/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/settings/settings.h"

#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_feature.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"

namespace brave_ads {

bool UserHasJoinedBraveRewards() {
  return GetProfileBooleanPref(brave_rewards::prefs::kEnabled);
}

bool UserHasOptedInToBraveNewsAds() {
  return GetProfileBooleanPref(brave_news::prefs::kBraveNewsOptedIn) &&
         GetProfileBooleanPref(brave_news::prefs::kNewTabPageShowToday);
}

bool UserHasOptedInToNewTabPageAds() {
  return GetProfileBooleanPref(
             ntp_background_images::prefs::kNewTabPageShowBackgroundImage) &&
         GetProfileBooleanPref(
             ntp_background_images::prefs::
                 kNewTabPageShowSponsoredImagesBackgroundImage);
}

bool UserHasOptedInToNotificationAds() {
  return UserHasJoinedBraveRewards() &&
         GetProfileBooleanPref(prefs::kOptedInToNotificationAds);
}

int GetMaximumNotificationAdsPerHour() {
  const int ads_per_hour = static_cast<int>(
      GetProfileInt64Pref(prefs::kMaximumNotificationAdsPerHour));

  return ads_per_hour > 0 ? ads_per_hour : kDefaultNotificationAdsPerHour.Get();
}

bool UserHasOptedInToSearchResultAds() {
  return GetProfileBooleanPref(prefs::kOptedInToSearchResultAds);
}

}  // namespace brave_ads
