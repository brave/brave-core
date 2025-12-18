/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_FEATURE_H_
#define BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_FEATURE_H_

#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"

namespace metrics {

BASE_DECLARE_FEATURE(kSearchQueryMetricsFeature);

// Whether to report metrics for non-regular profiles (e.g., incognito).
inline constexpr base::FeatureParam<bool> kShouldReportForNonRegularProfile{
    &kSearchQueryMetricsFeature, "should_report_for_non_regular_profile",
    false};

// Whether to retry reporting a metric after a failure.
inline constexpr base::FeatureParam<bool> kShouldRetryFailedReports{
    &kSearchQueryMetricsFeature, "should_retry_failed_reports", true};

// Initial delay between retry attempts when reporting a metric fails.
inline constexpr base::FeatureParam<base::TimeDelta> kInitialBackoffDelay{
    &kSearchQueryMetricsFeature, "initial_backoff_delay", base::Minutes(15)};

// Maximum delay between retry attempts when reporting a metric fails.
inline constexpr base::FeatureParam<base::TimeDelta> kMaxBackoffDelay{
    &kSearchQueryMetricsFeature, "max_backoff_delay", base::Days(1)};

// Maximum number of retry attempts when reporting a metric fails.
inline constexpr base::FeatureParam<int> kMaxRetryCount{
    &kSearchQueryMetricsFeature, "max_retry_count", 5};

}  // namespace metrics

#endif  // BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_SEARCH_QUERY_METRICS_FEATURE_H_
