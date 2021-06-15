/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_PREF_NAMES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_PREF_NAMES_H_

#include <cstdint>

namespace ads {

// Ads per hour are user configurable within the brave://rewards ads UI
const int64_t kMinimumAdNotificationsPerHour = 0;
const int64_t kMaximumAdNotificationsPerHour = 10;
const int64_t kDefaultAdNotificationsPerHour = 5;

namespace prefs {

extern const char kEnabled[];

extern const char kShouldAllowConversionTracking[];

extern const char kAdsPerHour[];

extern const char kIdleTimeThreshold[];

extern const char kShouldAllowAdsSubdivisionTargeting[];
extern const char kAdsSubdivisionTargetingCode[];
extern const char kAutoDetectedAdsSubdivisionTargetingCode[];

extern const char kCatalogId[];
extern const char kCatalogVersion[];
extern const char kCatalogPing[];
extern const char kCatalogLastUpdated[];

extern const char kEpsilonGreedyBanditArms[];
extern const char kEpsilonGreedyBanditEligibleSegments[];

extern const char kHasMigratedConversionState[];

}  // namespace prefs

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_PREF_NAMES_H_
