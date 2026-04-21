/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_

#include <cstddef>
#include <memory>

#include "base/containers/flat_map.h"
#include "brave/components/serp_metrics/serp_metric_type.h"
#include "brave/components/serp_metrics/serp_metrics_day_boundary_calculator.h"

class PrefService;
class TimePeriodStorage;
class TimePeriodStoreFactory;

namespace serp_metrics {

// SerpMetrics records and aggregates search engine usage counts.
//
// Counts are exposed for two reporting windows, based on the timestamp of the
// last successful usage ping (i.e., only searches not yet reported):
//  - Yesterday: searches from the most recent completed calendar day
//    (00:00:00 to 23:59:59 in the reporting timezone).
//  - Stale period: searches older than yesterday (but still within the
//    `TimePeriodStorage` retention window).
//
// Day boundaries are computed in UTC when `report_in_utc` is true, or in local
// time otherwise.

class SerpMetrics final {
 public:
  SerpMetrics(PrefService* local_state,
              const TimePeriodStoreFactory& time_period_store_factory,
              bool report_in_utc);

  SerpMetrics(const SerpMetrics&) = delete;
  SerpMetrics& operator=(const SerpMetrics&) = delete;

  ~SerpMetrics();

  void RecordSearch(SerpMetricType type);
  size_t GetSearchCountForYesterday(SerpMetricType type) const;

  size_t GetSearchCountForStalePeriod() const;

  void ClearHistory();

  // Test helpers to return the total search count stored in `TimePeriodStorage`
  // without filtering by time range or staleness.
  size_t GetSearchCountForTesting(SerpMetricType type) const;

 private:
  SerpMetricsDayBoundaryCalculator day_boundary_calculator_;

  base::flat_map<SerpMetricType, std::unique_ptr<TimePeriodStorage>>
      time_period_storages_;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_
