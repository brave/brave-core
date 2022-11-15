/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_COMMON_CONSTANTS_H_

#include <cstdint>

// TODO(https://github.com/brave/brave-browser/issues/13793): Rename ads
// namespace to brave_ads when bat-native-ads is moved to components/brave_ads
namespace ads {

// Brave Ads current version number
constexpr int kCurrentVersionNumber = 12;

// Brave Ads per hour are user configurable within the brave://rewards ads UI
constexpr int64_t kMinimumNotificationAdsPerHour = 0;
constexpr int64_t kMaximumNotificationAdsPerHour = 10;
constexpr int64_t kDefaultNotificationAdsPerHour = 5;

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_COMMON_CONSTANTS_H_
