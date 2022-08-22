/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_PREF_NAMES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_PREF_NAMES_H_

#include <cstdint>

namespace ads {

// Ads per hour are user configurable within the brave://rewards ads UI
const int64_t kMinimumNotificationAdsPerHour = 0;
const int64_t kMaximumNotificationAdsPerHour = 10;
const int64_t kDefaultNotificationAdsPerHour = 5;

namespace prefs {

extern const char kEnabled[];

extern const char kShouldAllowConversionTracking[];

extern const char kMaximumNotificationAdsPerHour[];

extern const char kIdleTimeThreshold[];

extern const char kShouldAllowSubdivisionTargeting[];
extern const char kSubdivisionTargetingCode[];
extern const char kAutoDetectedSubdivisionTargetingCode[];

extern const char kCatalogId[];
extern const char kCatalogVersion[];
extern const char kCatalogPing[];
extern const char kCatalogLastUpdated[];

extern const char kIssuerPing[];
extern const char kIssuers[];

extern const char kEpsilonGreedyBanditArms[];
extern const char kEpsilonGreedyBanditEligibleSegments[];

extern const char kNotificationAds[];

extern const char kServeAdAt[];

extern const char kNextTokenRedemptionAt[];

extern const char kHasMigratedClientState[];
extern const char kHasMigratedConfirmationState[];
extern const char kHasMigratedConversionState[];
extern const char kHasMigratedNotificationState[];
extern const char kHasMigratedRewardsState[];

extern const char kConfirmationsHash[];
extern const char kClientHash[];

extern const char kBrowserVersionNumber[];

}  // namespace prefs
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_PREF_NAMES_H_
