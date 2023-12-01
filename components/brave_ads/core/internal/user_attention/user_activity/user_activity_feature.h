/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_USER_ACTIVITY_USER_ACTIVITY_FEATURE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_USER_ACTIVITY_USER_ACTIVITY_FEATURE_H_

#include <string>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace base {
class TimeDelta;
}  // namespace base

namespace brave_ads {

BASE_DECLARE_FEATURE(kUserActivityFeature);

inline constexpr base::FeatureParam<int> kMaximumUserActivityEvents{
    &kUserActivityFeature, "maximum_events", 3600};

inline constexpr base::FeatureParam<std::string> kUserActivityTriggers{
    &kUserActivityFeature, "triggers",
    "0D0B14110D0B14110D0B14110D0B1411=-1.0;0D0B1411070707=-1.0;07070707=-1.0"};

inline constexpr base::FeatureParam<base::TimeDelta> kUserActivityTimeWindow{
    &kUserActivityFeature, "time_window", base::Minutes(15)};

inline constexpr base::FeatureParam<double> kUserActivityThreshold{
    &kUserActivityFeature, "threshold", 0.0};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ATTENTION_USER_ACTIVITY_USER_ACTIVITY_FEATURE_H_
