/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_COMMON_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_COMMON_PREF_NAMES_H_

namespace brave_ads::prefs {

// Brave Ads version pref
extern const char kVersion[];

// p2a and p3a prefs
extern const char kAdsWereDisabled[];
extern const char kHasAdsP3AState[];
extern const char kP2AStoragePrefNamePrefix[];

// Notification prefs
extern const char kShouldShowOnboardingNotification[];
extern const char kNotificationAdLastScreenPositionX[];
extern const char kNotificationAdLastScreenPositionY[];
extern const char kNotificationAdDidFallbackToCustom[];

// Migration prefs
extern const char kSupportedCountryCodesLastSchemaVersion[];

}  // namespace brave_ads::prefs

// TODO(https://github.com/brave/brave-browser/issues/13793): Rename ads::prefs
// namespace to brave_ads::prefs when bat-native-ads is moved to
// components/brave_ads
namespace ads::prefs {

// Brave Ads enabled/disabled pref
extern const char kEnabled[];

// Notification prefs
extern const char kMaximumNotificationAdsPerHour[];
extern const char kNotificationAds[];
extern const char kServeAdAt[];
extern const char kBrowserVersionNumber[];

// Idle state prefs
extern const char kIdleTimeThreshold[];

// Subdivision targeting prefs
extern const char kShouldAllowSubdivisionTargeting[];
extern const char kSubdivisionTargetingCode[];
extern const char kAutoDetectedSubdivisionTargetingCode[];

// Catalog prefs
extern const char kCatalogId[];
extern const char kCatalogVersion[];
extern const char kCatalogPing[];
extern const char kCatalogLastUpdated[];

// Issuer prefs
extern const char kIssuerPing[];
extern const char kIssuers[];

// Epsilon greedy bandit prefs
extern const char kEpsilonGreedyBanditArms[];
extern const char kEpsilonGreedyBanditEligibleSegments[];

// Unblinded token prefs
extern const char kNextTokenRedemptionAt[];

// Migration prefs
extern const char kHasMigratedClientState[];
extern const char kHasMigratedConfirmationState[];
extern const char kHasMigratedConversionState[];
extern const char kHasMigratedNotificationState[];
extern const char kHasMigratedRewardsState[];

// Confirmations prefs
extern const char kConfirmationsHash[];

// Client prefs
extern const char kClientHash[];

}  // namespace ads::prefs

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_COMMON_PREF_NAMES_H_
