/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_

#include <cstddef>
#include <memory>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ref.h"
#include "brave/components/serp_metrics/serp_metric_type.h"

class PrefService;

namespace base {
class Time;
}  // namespace base

namespace serp_metrics {

class SerpMetricsTimePeriodStorage;
class SerpMetricsTimePeriodStoreFactory;

// `SerpMetrics` records and aggregates search engine usage counts.
//
// Counts are exposed for two reporting windows, based on `kLastReportedAt`
// (i.e., only searches not yet included in the last report):
//  - Yesterday: searches from the most recent completed calendar day
//    (00:00:00 to 23:59:59 UTC).
//  - Stale period: searches older than yesterday (but still within the
//    `TimePeriodStorage` retention window).

class SerpMetrics final {
 public:
  SerpMetrics(
      PrefService* local_state,
      const SerpMetricsTimePeriodStoreFactory& time_period_store_factory);

  SerpMetrics(const SerpMetrics&) = delete;
  SerpMetrics& operator=(const SerpMetrics&) = delete;

  ~SerpMetrics();

  void RecordSearch(SerpMetricType type);
  size_t GetSearchCountForYesterday(SerpMetricType type) const;

  size_t GetSearchCountForStalePeriod() const;

  void ClearHistory();

  // Test helpers to return the total search count stored in
  // `SerpMetricsTimePeriodStorage` without filtering by time range or
  // staleness.
  size_t GetSearchCountForTesting(SerpMetricType type) const;

 private:
  // Returns the start of the stale period in UTC, based on the last day
  // metrics were reported. Searches recorded since that day have not yet been
  // reported, so the stale period begins at UTC midnight of that day. If the
  // last reported date is unavailable or invalid, an empty time is returned to
  // indicate that the full retention period should be considered stale.
  base::Time GetStartOfStalePeriod() const;

  const raw_ref<PrefService> local_state_;

  base::flat_map<SerpMetricType, std::unique_ptr<SerpMetricsTimePeriodStorage>>
      time_period_storages_;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_
