/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_UNITTEST_UTIL_H_

namespace brave_ads {

class AdType;
struct AdInfo;

void DisableBraveRewardsForTesting();

void DisableBraveNewsAdsForTesting();
void DisableNewTabPageAdsForTesting();
void DisableNotificationAdsForTesting();

AdInfo BuildAdForTesting(const AdType& ad_type, bool should_use_random_uuids);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_UNITTEST_UTIL_H_
