/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_NAMES_H_

namespace brave_ads::prefs {

// Ads prefs.
inline constexpr char kDiagnosticId[] = "brave.brave_ads.diagnostics.id";

inline constexpr char kOptedInToNotificationAds[] = "brave.brave_ads.enabled";
inline constexpr char kMaximumNotificationAdsPerHour[] =
    "brave.brave_ads.ads_per_hour";

inline constexpr char kOptedInToSearchResultAds[] =
    "brave.brave_ads.opted_in_to_search_result_ads";

inline constexpr char kShouldAllowSubdivisionTargeting[] =
    "brave.brave_ads.should_allow_ads_subdivision_targeting";
inline constexpr char kSubdivisionTargetingUserSelectedSubdivision[] =
    "brave.brave_ads.ads_subdivision_targeting_code";
inline constexpr char kSubdivisionTargetingAutoDetectedSubdivision[] =
    "brave.brave_ads.automatically_detected_ads_subdivision_targeting_code";

inline constexpr char kCatalogId[] = "brave.brave_ads.catalog_id";
inline constexpr char kCatalogVersion[] = "brave.brave_ads.catalog_version";
inline constexpr char kCatalogPing[] = "brave.brave_ads.catalog_ping";
inline constexpr char kCatalogLastUpdated[] =
    "brave.brave_ads.catalog_last_updated";

inline constexpr char kIssuerPing[] = "brave.brave_ads.issuer_ping";
inline constexpr char kIssuers[] = "brave.brave_ads.issuers";

inline constexpr char kNotificationAds[] = "brave.brave_ads.notification_ads";
inline constexpr char kServeAdAt[] = "brave.brave_ads.serve_ad_at";

inline constexpr char kNextTokenRedemptionAt[] =
    "brave.brave_ads.rewards.next_time_redemption_at";

inline constexpr char kAdReactions[] = "brave.brave_ads.reactions.ads";
inline constexpr char kSegmentReactions[] =
    "brave.brave_ads.reactions.segments";
inline constexpr char kSaveAds[] = "brave.brave_ads.reactions.saved_ads";
inline constexpr char kMarkedAsInappropriate[] =
    "brave.brave_ads.reactions.marked_as_inappropriate";

inline constexpr char kHasMigratedClientState[] =
    "brave.brave_ads.state.has_migrated.client.v6";
inline constexpr char kHasMigratedConfirmationState[] =
    "brave.brave_ads.state.has_migrated.confirmations.v8";
inline constexpr char kShouldMigrateVerifiedRewardsUser[] =
    "brave.brave_ads.rewards.verified_user.should_migrate";

inline constexpr char kBrowserVersionNumber[] =
    "brave.brave_ads.browser_version_number";

// Ads service prefs.
inline constexpr char kNotificationAdLastNormalizedCoordinateX[] =
    "brave.brave_ads.ad_notification.last_normalized_coordinate_x";
inline constexpr char kNotificationAdLastNormalizedCoordinateY[] =
    "brave.brave_ads.ad_notification.last_normalized_coordinate_y";
inline constexpr char kNotificationAdDidFallbackToCustom[] =
    "brave.brave_ads.ad_notification.did_fallback_to_custom";

inline constexpr char kShouldShowOnboardingNotification[] =
    "brave.brave_ads.should_show_my_first_ad_notification";

// Brave stats prefs.
inline constexpr char kEnabledForLastProfile[] =
    "brave.brave_ads.enabled_last_profile";
inline constexpr char kEverEnabledForAnyProfile[] =
    "brave.brave_ads.ever_enabled_any_profile";

// P2A prefs.
inline constexpr char kP2APrefPathPrefix[] = "brave.weekly_storage.";

}  // namespace brave_ads::prefs

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_PREFS_PREF_NAMES_H_
