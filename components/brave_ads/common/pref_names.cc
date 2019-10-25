/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/common/pref_names.h"

namespace brave_ads {

namespace prefs {

// Stores whether Brave ads is enabled or disabled
const char kEnabled[] = "brave.brave_ads.enabled";

// Stores whether ads were disabled at least once
const char kBraveAdsWereDisabled[] = "brave.brave_ads.were_disabled";

// Stores the maximum amount of ads per hour
const char kAdsPerHour[] = "brave.brave_ads.ads_per_hour";

// Stores the maximum amount of ads per day
const char kAdsPerDay[] = "brave.brave_ads.ads_per_day";

// Stores the idle threshold before checking if an ad can be served
const char kIdleThreshold[] = "brave.brave_ads.idle_threshold";

// Stores whether onboarding should be shown
const char kShouldShowOnboarding[] =
    "brave.brave_ads.should_show_first_launch_notification";

// Stores the timestamp of when onboarding was shown
const char kOnboardingTimestamp[] =
    "brave.brave_ads.launch_notification_timestamp";

// Stores whether we should show the My First Ad notification
const char kShouldShowMyFirstAdNotification[] =
    "brave.brave_ads.should_show_my_first_ad_notification";

// Stores the preferences version number
const char kVersion[] = "brave.brave_ads.prefs.version";

const int kCurrentVersionNumber = 4;

}  // namespace prefs

}  // namespace brave_ads
