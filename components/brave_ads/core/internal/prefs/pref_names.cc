/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/prefs/pref_names.h"  // IWYU pragma: keep

namespace brave_ads::prefs {

// Stores the preferences version number
const char kVersion[] = "brave.brave_ads.prefs.current_version";

// Prefix for preference names pertaining to p2a weekly metrics
const char kP2APrefPathPrefix[] = "brave.weekly_storage.";

// Stores whether we should show the My First notification ad
const char kShouldShowOnboardingNotification[] =
    "brave.brave_ads.should_show_my_first_ad_notification";

// Stores the last normalized screen position of custom notification ads and
// whether to fallback from native to custom notification ads if native
// notifications are disabled
const char kNotificationAdLastNormalizedDisplayCoordinateX[] =
    "brave.brave_ads.ad_notification.last_normalized_display_coordinate_x";
const char kNotificationAdLastNormalizedDisplayCoordinateY[] =
    "brave.brave_ads.ad_notification.last_normalized_display_coordinate_y";
const char kNotificationAdDidFallbackToCustom[] =
    "brave.brave_ads.ad_notification.did_fallback_to_custom";

// Stores the supported country codes current schema version number
const char kSupportedCountryCodesLastSchemaVersion[] =
    "brave.brave_ads.supported_regions_last_schema_version_number";

// Stores whether user has opted-in to notifications ads
const char kOptedInToNotificationAds[] = "brave.brave_ads.enabled";
const char kEnabledForLastProfile[] = "brave.brave_ads.enabled_last_profile";
const char kEverEnabledForAnyProfile[] =
    "brave.brave_ads.ever_enabled_any_profile";

// Stores a diagnostic id
const char kDiagnosticId[] = "brave.brave_ads.diagnostics.id";

// Stores the maximum number of notification ads per hour
const char kMaximumNotificationAdsPerHour[] = "brave.brave_ads.ads_per_hour";

// Notification ads
const char kNotificationAds[] = "brave.brave_ads.notification_ads";
const char kServeAdAt[] = "brave.brave_ads.serve_ad_at";

// Browser version
const char kBrowserVersionNumber[] = "brave.brave_ads.browser_version_number";

// Stores whether Brave ads should allow subdivision ad targeting
const char kShouldAllowSubdivisionTargeting[] =
    "brave.brave_ads.should_allow_ads_subdivision_targeting";

// Stores the selected subdivision targeting code
const char kSubdivisionTargetingSubdivision[] =
    "brave.brave_ads.ads_subdivision_targeting_code";

// Stores the automatically detected subdivision targeting code
const char kSubdivisionTargetingAutoDetectedSubdivision[] =
    "brave.brave_ads.automatically_detected_ads_subdivision_targeting_code";

// Stores catalog id
const char kCatalogId[] = "brave.brave_ads.catalog_id";

// Stores catalog version
const char kCatalogVersion[] = "brave.brave_ads.catalog_version";

// Stores catalog ping
const char kCatalogPing[] = "brave.brave_ads.catalog_ping";

// Stores catalog last updated
const char kCatalogLastUpdated[] = "brave.brave_ads.catalog_last_updated";

// Stores issuers
const char kIssuerPing[] = "brave.brave_ads.issuer_ping";
const char kIssuers[] = "brave.brave_ads.issuers";

// Stores epsilon greedy bandit
const char kEpsilonGreedyBanditArms[] =
    "brave.brave_ads.epsilon_greedy_bandit_arms.v2";
const char kEpsilonGreedyBanditEligibleSegments[] =
    "brave.brave_ads.epsilon_greedy_bandit_eligible_segments.v2";

// Rewards
const char kNextTokenRedemptionAt[] =
    "brave.brave_ads.rewards.next_time_redemption_at";

// Stores migration status
const char kHasMigratedClientState[] =
    "brave.brave_ads.state.has_migrated.client.v5";
const char kHasMigratedConfirmationState[] =
    "brave.brave_ads.state.has_migrated.confirmations.v5";
const char kHasMigratedConversionState[] =
    "brave.brave_ads.migrated.conversion_state";
const char kHasMigratedNotificationState[] =
    "brave.brave_ads.has_migrated.notification_state";
const char kHasMigratedRewardsState[] =
    "brave.brave_ads.migrated.rewards_state";
const char kShouldMigrateVerifiedRewardsUser[] =
    "brave.brave_ads.rewards.verified_user.should_migrate";

}  // namespace brave_ads::prefs
