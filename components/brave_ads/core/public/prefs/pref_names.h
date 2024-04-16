/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_NAMES_H_

namespace brave_ads::prefs {

// Stores the preferences version number
inline constexpr char kVersion[] = "brave.brave_ads.prefs.current_version";

// Prefix for preference names pertaining to p2a weekly metrics
inline constexpr char kP2APrefPathPrefix[] = "brave.weekly_storage.";

// Stores whether we should show the My First notification ad
inline constexpr char kShouldShowOnboardingNotification[] =
    "brave.brave_ads.should_show_my_first_ad_notification";

// Stores the last normalized screen position of custom notification ads and
// whether to fallback from native to custom notification ads if native
// notifications are disabled
inline constexpr char kNotificationAdLastNormalizedCoordinateX[] =
    "brave.brave_ads.ad_notification.last_normalized_coordinate_x";
inline constexpr char kNotificationAdLastNormalizedCoordinateY[] =
    "brave.brave_ads.ad_notification.last_normalized_coordinate_y";
inline constexpr char kNotificationAdDidFallbackToCustom[] =
    "brave.brave_ads.ad_notification.did_fallback_to_custom";

// Stores the supported country codes current schema version number
inline constexpr char kSupportedCountryCodesLastSchemaVersion[] =
    "brave.brave_ads.supported_regions_last_schema_version_number";

// Stores whether user has opted-in to notifications ads
inline constexpr char kOptedInToNotificationAds[] = "brave.brave_ads.enabled";
inline constexpr char kEnabledForLastProfile[] =
    "brave.brave_ads.enabled_last_profile";
inline constexpr char kEverEnabledForAnyProfile[] =
    "brave.brave_ads.ever_enabled_any_profile";

// Stores a diagnostic id
inline constexpr char kDiagnosticId[] = "brave.brave_ads.diagnostics.id";

// Stores the maximum number of notification ads per hour
inline constexpr char kMaximumNotificationAdsPerHour[] =
    "brave.brave_ads.ads_per_hour";

// Notification ads
inline constexpr char kNotificationAds[] = "brave.brave_ads.notification_ads";
inline constexpr char kServeAdAt[] = "brave.brave_ads.serve_ad_at";

// Browser version
inline constexpr char kBrowserVersionNumber[] =
    "brave.brave_ads.browser_version_number";

// Stores whether Brave ads should allow subdivision ad targeting
inline constexpr char kShouldAllowSubdivisionTargeting[] =
    "brave.brave_ads.should_allow_ads_subdivision_targeting";

// Stores the selected subdivision targeting code
inline constexpr char kSubdivisionTargetingSubdivision[] =
    "brave.brave_ads.ads_subdivision_targeting_code";

// Stores the automatically detected subdivision targeting code
inline constexpr char kSubdivisionTargetingAutoDetectedSubdivision[] =
    "brave.brave_ads.automatically_detected_ads_subdivision_targeting_code";

// Stores catalog id
inline constexpr char kCatalogId[] = "brave.brave_ads.catalog_id";

// Stores catalog version
inline constexpr char kCatalogVersion[] = "brave.brave_ads.catalog_version";

// Stores catalog ping
inline constexpr char kCatalogPing[] = "brave.brave_ads.catalog_ping";

// Stores catalog last updated
inline constexpr char kCatalogLastUpdated[] =
    "brave.brave_ads.catalog_last_updated";

// Stores issuers
inline constexpr char kIssuerPing[] = "brave.brave_ads.issuer_ping";
inline constexpr char kIssuers[] = "brave.brave_ads.issuers";

// Stores epsilon greedy bandit
inline constexpr char kEpsilonGreedyBanditArms[] =
    "brave.brave_ads.epsilon_greedy_bandit_arms.v2";
inline constexpr char kEpsilonGreedyBanditEligibleSegments[] =
    "brave.brave_ads.epsilon_greedy_bandit_eligible_segments.v2";

// Rewards
inline constexpr char kNextTokenRedemptionAt[] =
    "brave.brave_ads.rewards.next_time_redemption_at";

// Stores migration status
inline constexpr char kHasMigratedClientState[] =
    "brave.brave_ads.state.has_migrated.client.v6";
inline constexpr char kHasMigratedConfirmationState[] =
    "brave.brave_ads.state.has_migrated.confirmations.v8";
inline constexpr char kHasMigratedConversionState[] =
    "brave.brave_ads.migrated.conversion_state";
inline constexpr char kHasMigratedNotificationState[] =
    "brave.brave_ads.has_migrated.notification_state";
inline constexpr char kHasMigratedRewardsState[] =
    "brave.brave_ads.migrated.rewards_state";
inline constexpr char kShouldMigrateVerifiedRewardsUser[] =
    "brave.brave_ads.rewards.verified_user.should_migrate";

}  // namespace brave_ads::prefs

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_NAMES_H_
