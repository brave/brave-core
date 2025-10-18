/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"

#include "brave/components/brave_ads/core/internal/common/test/profile_pref_value_test_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"

namespace brave_ads::test {

void DisableBraveRewards() {
  SetProfileBooleanPrefValue(brave_rewards::prefs::kEnabled, false);
  DisconnectExternalBraveRewardsWallet();
}

void DisconnectExternalBraveRewardsWallet() {
  SetProfileStringPrefValue(brave_rewards::prefs::kExternalWalletType, "");
}

void OptOutOfNewTabPageAds() {
  SetProfileBooleanPrefValue(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, false);
  SetProfileBooleanPrefValue(ntp_background_images::prefs::
                                 kNewTabPageShowSponsoredImagesBackgroundImage,
                             false);
}

void OptOutOfNotificationAds() {
  SetProfileBooleanPrefValue(prefs::kOptedInToNotificationAds, false);
}

void SetMaximumNotificationAdsPerHour(int max_ads_per_hour) {
  SetProfileInt64PrefValue(prefs::kMaximumNotificationAdsPerHour,
                           max_ads_per_hour);
}

void OptOutOfSearchResultAds() {
  SetProfileBooleanPrefValue(prefs::kOptedInToSearchResultAds, false);
}

void OptOutOfAllAds() {
  OptOutOfNewTabPageAds();
  OptOutOfNotificationAds();
  OptOutOfSearchResultAds();
}

void OptOutOfSurveyPanelist() {
  SetProfileBooleanPrefValue(
      ntp_background_images::prefs::kNewTabPageSponsoredImagesSurveyPanelist,
      false);
}

}  // namespace brave_ads::test
