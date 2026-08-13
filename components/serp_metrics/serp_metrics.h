/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_

#include <cstddef>
#include <memory>
#include <optional>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ref.h"
#include "base/time/time.h"
#include "brave/components/serp_metrics/serp_metric_type.h"

class PrefService;

namespace serp_metrics {

class SerpMetricsTimePeriodStorage;
class SerpMetricsTimePeriodStoreFactory;

// `SerpMetrics` records and aggregates search engine usage counts.
//
// Counts are exposed for two reporting windows, based on a stale-period
// cutoff (derived from `kLastReportedAt` by default, or from an explicit
// `last_report_time` parameter when provided):
//  - Yesterday: searches from the most recent completed calendar day
//    (00:00:00 to 23:59:59 UTC), adjusted so that searches already covered by
//    the cutoff are not re-counted.
//  - Stale period: searches older than yesterday (but still within the
//    `TimePeriodStorage` retention window) and not yet covered by the cutoff.
class SerpMetrics final {
 public:
  SerpMetrics(
      PrefService* local_state,
      const SerpMetricsTimePeriodStoreFactory& time_period_store_factory);

  SerpMetrics(const SerpMetrics&) = delete;
  SerpMetrics& operator=(const SerpMetrics&) = delete;

  ~SerpMetrics();

  void RecordSearch(SerpMetricType type);

  // Returns the count for yesterday (the most recent completed UTC calendar
  // day), excluding any portion already covered by the stale-period cutoff.
  // When `last_report_time` is provided, it overrides the default
  // `kLastReportedAt`-based cutoff. A null `base::Time` means nothing has been
  // reported yet, so the entire retention period is considered stale.
  size_t GetSearchCountForYesterday(
      SerpMetricType type,
      std::optional<base::Time> last_report_time = std::nullopt) const;

  // Returns the total search count across all engine types for the stale
  // period (older than yesterday, within retention, not yet reported).
  // Uses the same cutoff semantics as `GetSearchCountForYesterday`.
  size_t GetSearchCountForStalePeriod(
      std::optional<base::Time> last_report_time = std::nullopt) const;

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
  base::Time GetStartOfStalePeriod(
      std::optional<base::Time> last_report_time) const;

  const raw_ref<PrefService> local_state_;

  base::flat_map<SerpMetricType, std::unique_ptr<SerpMetricsTimePeriodStorage>>
      time_period_storages_;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_
