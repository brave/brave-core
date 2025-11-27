/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_OBLIVIOUS_HTTP_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_OBLIVIOUS_HTTP_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

BASE_DECLARE_FEATURE(kAdsObliviousHttpFeature);

// Controls whether Oblivious HTTP (OHTTP) is enabled for requests. When
// enabled, eligible network requests will be sent using OHTTP.
inline constexpr base::FeatureParam<bool> kShouldSupportOhttp{
    &kAdsObliviousHttpFeature, "should_support", true};

// Because OHTTP requests are routed through a relay and gateway, overall
// latency may increase. This timeout prevents requests from hanging on slow or
// unreliable networks.
inline constexpr base::FeatureParam<base::TimeDelta> kOhttpTimeoutDuration{
    &kAdsObliviousHttpFeature, "timeout_duration", base::Seconds(3)};

// Cached OHTTP key configs expire after this duration. Once expired, a fresh
// key config will be fetched.
inline constexpr base::FeatureParam<base::TimeDelta>
    kOhttpKeyConfigExpiresAfter{&kAdsObliviousHttpFeature,
                                "key_config_expires_after", base::Days(3)};

// Initial delay before retrying a failed attempt to fetch the OHTTP key config.
// Subsequent failures apply exponential backoff.
inline constexpr base::FeatureParam<base::TimeDelta>
    kOhttpKeyConfigInitialBackoffDelay{&kAdsObliviousHttpFeature,
                                       "key_config_initial_backoff_delay",
                                       base::Minutes(5)};

// Maximum delay allowed between retries when fetching the OHTTP key config
// continues to fail.
inline constexpr base::FeatureParam<base::TimeDelta>
    kOhttpKeyConfigMaxBackoffDelay{&kAdsObliviousHttpFeature,
                                   "key_config_max_backoff_delay",
                                   base::Days(1)};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_OBLIVIOUS_HTTP_FEATURE_H_
