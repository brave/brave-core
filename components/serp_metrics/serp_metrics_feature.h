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

// Uses `size_t` days rather than `base::TimeDelta` because `TimePeriodStorage`
// does not support `TimeDelta` field trial parameters. Two 31-day months ensure
// the previous calendar month always falls within the retention window when the
// monthly ping fires, since the worst case is a full 31-day prior month plus
// up to 31 days into the current month.
inline constexpr base::FeatureParam<size_t> kSerpMetricsTimePeriodInDays{
    &kSerpMetricsFeature, "time_period_in_days", 2 * 31};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_FEATURE_H_
