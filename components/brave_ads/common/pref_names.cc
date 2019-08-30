/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/common/pref_names.h"

namespace brave_ads {

namespace prefs {

const char kEnabled[] = "brave.brave_ads.enabled";

const char kAdsPerHour[] = "brave.brave_ads.ads_per_hour";
const char kAdsPerDay[] = "brave.brave_ads.ads_per_day";
const char kAdsPerSameTime[] = "brave.brave_ads.ads_per_same_time";

const char kIdleThreshold[] = "brave.brave_ads.idle_threshold";

const char kShouldShowFirstLaunchNotification[] =
    "brave.brave_ads.should_show_first_launch_notification";
const char kHasRemovedFirstLaunchNotification[] =
    "brave.brave_ads.has_removed_first_launch_notification";
const char kLastShownFirstLaunchNotificationTimestamp[] =
    "brave.brave_ads.launch_notification_timestamp";

const char kShouldShowMyFirstAdNotification[] =
    "brave.brave_ads.should_show_my_first_ad_notification";

const int kCurrentVersionNumber = 3;
const char kVersion[] = "brave.brave_ads.prefs.version";

}  // namespace prefs

}  // namespace brave_ads
