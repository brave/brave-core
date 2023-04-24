/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_COMMON_CONSTANTS_H_

#include <cstdint>

namespace brave_ads {

constexpr int kCurrentPrefVersion = 12;

// Brave Ads per hour are user configurable within the brave://rewards ads UI
constexpr int64_t kDefaultBraveRewardsNotificationAdsPerHour = 10;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_COMMON_CONSTANTS_H_
