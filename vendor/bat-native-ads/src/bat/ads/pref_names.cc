/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/pref_names.h"

namespace ads {
namespace prefs {

// Stores whether Brave ads is enabled or disabled
const char kEnabled[] = "brave.brave_ads.enabled";

// Stores whether Brave ads should allow conversion tracking
const char kShouldAllowConversionTracking[] =
    "brave.brave_ads.should_allow_ad_conversion_tracking";

// Stores the maximum number of notification ads per hour
const char kMaximumNotificationAdsPerHour[] = "brave.brave_ads.ads_per_hour";

// Stores the idle time threshold before checking if an ad can be served
const char kIdleTimeThreshold[] = "brave.brave_ads.idle_threshold";

// Stores whether Brave ads should allow subdivision ad targeting
const char kShouldAllowSubdivisionTargeting[] =
    "brave.brave_ads.should_allow_ads_subdivision_targeting";

// Stores the selected subdivision targeting code
const char kSubdivisionTargetingCode[] =
    "brave.brave_ads.ads_subdivision_targeting_code";

// Stores the automatically detected subdivision targeting code
const char kAutoDetectedSubdivisionTargetingCode[] =
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

// Ads
const char kServeAdAt[] = "brave.brave_ads.serve_ad_at";

// Rewards
const char kNextTokenRedemptionAt[] =
    "brave.brave_ads.rewards.next_time_redemption_at";

// Stores migration status
const char kHasMigratedClientState[] =
    "brave.brave_ads.has_migrated.client_state";
const char kHasMigratedConfirmationState[] =
    "brave.brave_ads.has_migrated.confirmation_state";
const char kHasMigratedConversionState[] =
    "brave.brave_ads.migrated.conversion_state";
const char kHasMigratedRewardsState[] =
    "brave.brave_ads.migrated.rewards_state";

const char kConfirmationsHash[] = "brave.brave_ads.confirmations.hash.v2";
const char kClientHash[] = "brave.brave_ads.client.hash.v3";

}  // namespace prefs
}  // namespace ads
