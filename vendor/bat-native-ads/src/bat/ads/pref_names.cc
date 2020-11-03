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

// Stores the maximum amount of ads per day
const char kAdsPerDay[] = "brave.brave_ads.ads_per_day";

// Stores the idle threshold before checking if an ad can be served
const char kIdleThreshold[] = "brave.brave_ads.idle_threshold";

// Stores whether Brave ads should allow subdivision ad targeting
const char kShouldAllowAdsSubdivisionTargeting[] =
    "brave.brave_ads.should_allow_ads_subdivision_targeting";

// Stores the selected ads subdivision targeting code
const char kAdsSubdivisionTargetingCode[] =
    "brave.brave_ads.ads_subdivision_targeting_code";

// Stores the automatically detected ads subdivision targeting code
const char kAutoDetectedAdsSubdivisionTargetingCode[] =
    "brave.brave_ads.automatically_detected_ads_subdivision_targeting_code";

}  // namespace prefs

}  // namespace ads
