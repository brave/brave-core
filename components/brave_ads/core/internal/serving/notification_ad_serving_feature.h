/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace brave_ads {

BASE_DECLARE_FEATURE(kNotificationAdServingFeature);

inline constexpr base::FeatureParam<int> kNotificationAdServingVersion{
    &kNotificationAdServingFeature, "version", 2};

inline constexpr base::FeatureParam<base::TimeDelta>
    kServeFirstNotificationAdAfter{&kNotificationAdServingFeature,
                                   "serve_first_ad_after", base::Minutes(2)};

inline constexpr base::FeatureParam<base::TimeDelta>
    kMinimumDelayBeforeServingNotificationAd{
        &kNotificationAdServingFeature, "minimum_delay_before_serving_an_ad",
        base::Minutes(1)};

inline constexpr base::FeatureParam<base::TimeDelta>
    kRetryServingNotificationAdAfter{&kNotificationAdServingFeature,
                                     "retry_serving_ad_after",
                                     base::Minutes(2)};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_SERVING_NOTIFICATION_AD_SERVING_FEATURE_H_
