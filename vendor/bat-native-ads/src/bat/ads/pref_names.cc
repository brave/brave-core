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

// Stores the maximum amount of ads per hour
const char kAdsPerHour[] = "brave.brave_ads.ads_per_hour";

// Stores the idle time threshold before checking if an ad can be served
const char kIdleTimeThreshold[] = "brave.brave_ads.idle_threshold";

// Stores whether Brave ads should allow subdivision ad targeting
const char kShouldAllowAdsSubdivisionTargeting[] =
    "brave.brave_ads.should_allow_ads_subdivision_targeting";

// Stores the selected ads subdivision targeting code
const char kAdsSubdivisionTargetingCode[] =
    "brave.brave_ads.ads_subdivision_targeting_code";

// Stores the automatically detected ads subdivision targeting code
const char kAutoDetectedAdsSubdivisionTargetingCode[] =
    "brave.brave_ads.automatically_detected_ads_subdivision_targeting_code";

// Stores catalog id
const char kCatalogId[] = "brave.brave_ads.catalog_id";

// Stores catalog version
const char kCatalogVersion[] = "brave.brave_ads.catalog_version";

// Stores catalog ping
const char kCatalogPing[] = "brave.brave_ads.catalog_ping";

// Stores catalog last updated
const char kCatalogLastUpdated[] = "brave.brave_ads.catalog_last_updated";

// Stores epsilon greedy bandit arms
const char kEpsilonGreedyBanditArms[] =
    "brave.brave_ads.epsilon_greedy_bandit_arms";

// Stores epsilon greedy bandit eligible segments
const char kEpsilonGreedyBanditEligibleSegments[] =
    "brave.brave_ads.epsilon_greedy_bandit_eligible_segments";

const char kUnreconciledTransactions[] =
    "brave.brave_ads.ad_rewards.unreconciled_transactions";

// Stores migration status
const char kHasMigratedConversionState[] =
    "brave.brave_ads.migrated.conversion_state";

}  // namespace prefs

}  // namespace ads
