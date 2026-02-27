/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_AGGREGATOR_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_AGGREGATOR_H_

#include <cstddef>

#include "brave/components/serp_metrics/serp_metric_type.h"

namespace serp_metrics {

// An interface for classes that aggregate search engine usage counts.
//
// Counts are exposed for two reporting windows:
//  - Yesterday: searches from the most recent completed calendar day.
//  - Stale period: searches older than yesterday (within the retention window).
// See `SerpMetrics` class documentation for detailed definitions.
class SerpMetricsAggregator {
 public:
  virtual ~SerpMetricsAggregator() = default;

  virtual size_t GetSearchCountForYesterday(SerpMetricType type) const = 0;
  virtual size_t GetSearchCountForStalePeriod() const = 0;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_AGGREGATOR_H_
