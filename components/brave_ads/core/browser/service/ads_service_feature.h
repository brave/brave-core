/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_ADS_SERVICE_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_ADS_SERVICE_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/time/time.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kAdsServiceFeature);

// TODO(tmancey): Before merging set default to false.
inline constexpr base::FeatureParam<bool> kShouldSupportOhttp{
    &kAdsServiceFeature, "should_support_ohttp", true};

inline constexpr base::FeatureParam<base::TimeDelta> kOhttpTimeoutDuration{
    &kAdsServiceFeature, "ohttp_timeout_duration", base::Seconds(10)};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_ADS_SERVICE_FEATURE_H_
