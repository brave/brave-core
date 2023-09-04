/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_NAMES_H_

namespace brave_ads::prefs {

// Brave Ads version pref
extern const char kVersion[];

// p2a pref
extern const char kP2AStoragePrefNamePrefix[];

// Notification prefs
extern const char kShouldShowOnboardingNotification[];
extern const char kNotificationAdLastNormalizedDisplayCoordinateX[];
extern const char kNotificationAdLastNormalizedDisplayCoordinateY[];
extern const char kNotificationAdDidFallbackToCustom[];

// Migration prefs
extern const char kSupportedCountryCodesLastSchemaVersion[];

// Brave Ads enabled/disabled pref
extern const char kOptedInToNotificationAds[];
extern const char kEnabledForLastProfile[];
extern const char kEverEnabledForAnyProfile[];

// Diagnostic id prefs
extern const char kDiagnosticId[];

// Notification prefs
extern const char kMaximumNotificationAdsPerHour[];
extern const char kNotificationAds[];
extern const char kServeAdAt[];
extern const char kBrowserVersionNumber[];

// Subdivision targeting prefs
extern const char kShouldAllowSubdivisionTargeting[];
extern const char kSubdivisionTargetingSubdivision[];
extern const char kSubdivisionTargetingAutoDetectedSubdivision[];

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
extern const char kShouldMigrateVerifiedRewardsUser[];

}  // namespace brave_ads::prefs

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_NAMES_H_
