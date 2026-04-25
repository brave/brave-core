/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_FEATURE_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_FEATURE_H_

#include <cstddef>

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace serp_metrics {

BASE_DECLARE_FEATURE(kSerpMetricsFeature);

// Ideally this would be `base::FeatureParam<base::TimeDelta>`, but that type is
// not currently supported for `TimePeriodStorage`.
inline constexpr base::FeatureParam<size_t> kSerpMetricsTimePeriodInDays{
    &kSerpMetricsFeature, "time_period_in_days", 28};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_FEATURE_H_
