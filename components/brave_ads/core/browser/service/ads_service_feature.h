/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_ADS_SERVICE_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_ADS_SERVICE_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

BASE_DECLARE_FEATURE(kAdsServiceFeature);

inline constexpr base::FeatureParam<bool> kShouldSupportOhttp{
    &kAdsServiceFeature, "should_support_ohttp", true};

// Use a default 15-second timeout to account for the extra latency of OHTTP.
// OHTTP requests pass through a relay and gateway and take longer than normal
// HTTPS, so a longer timeout helps avoid failures on slower networks.
inline constexpr base::FeatureParam<base::TimeDelta> kOhttpTimeoutDuration{
    &kAdsServiceFeature, "ohttp_timeout_duration", base::Seconds(15)};

inline constexpr base::FeatureParam<base::TimeDelta> kFetchOhttpKeyConfigAfter{
    &kAdsServiceFeature, "fetch_ohttp_key_config_after", base::Days(1)};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_ADS_SERVICE_FEATURE_H_
