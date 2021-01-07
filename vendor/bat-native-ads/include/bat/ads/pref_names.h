/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_PREF_NAMES_H_
#define BAT_ADS_PREF_NAMES_H_

#include <stdint.h>

namespace ads {

// Ads per hour are user configurable with the brave://rewards ads UI
const uint64_t kMinimumAdNotificationsPerHour = 1;
const uint64_t kMaximumAdNotificationsPerHour = 5;
const uint64_t kDefaultAdNotificationsPerHour = 2;

namespace prefs {

extern const char kEnabled[];

extern const char kShouldAllowConversionTracking[];

extern const char kAdsPerHour[];

extern const char kIdleThreshold[];

extern const char kShouldAllowAdsSubdivisionTargeting[];
extern const char kAdsSubdivisionTargetingCode[];
extern const char kAutoDetectedAdsSubdivisionTargetingCode[];

extern const char kCatalogId[];
extern const char kCatalogVersion[];
extern const char kCatalogPing[];
extern const char kCatalogLastUpdated[];

extern const char kEpsilonGreedyBanditArms[];
extern const char kEpsilonGreedyBanditEligibleSegments[];

}  // namespace prefs

}  // namespace ads

#endif  // BAT_ADS_PREF_NAMES_H_
