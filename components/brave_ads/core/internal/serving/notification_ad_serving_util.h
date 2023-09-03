/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_UTIL_H_

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace brave_ads {

bool ShouldServeAdsAtRegularIntervals();

void SetServeAdAt(base::Time serve_ad_at);
base::Time ServeAdAt();

base::TimeDelta CalculateDelayBeforeServingAnAd();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_UTIL_H_
