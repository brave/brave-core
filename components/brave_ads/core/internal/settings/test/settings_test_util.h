/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SETTINGS_TEST_SETTINGS_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SETTINGS_TEST_SETTINGS_TEST_UTIL_H_

namespace brave_ads::test {

void DisableBraveRewards();
void DisconnectExternalBraveRewardsWallet();

void DisableNotificationAds();
void SetMaximumNotificationAdsPerHour(int max_ads_per_hour);

void DisableSponsoredAds();

void DisableNewTabPageBackgroundImages();

void DisableAllAds();

void OptOutOfSurveyPanelist();

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SETTINGS_TEST_SETTINGS_TEST_UTIL_H_
