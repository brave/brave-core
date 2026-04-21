/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_

#include <cstddef>
#include <memory>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/serp_metrics/serp_metric_type.h"

class PrefService;
class TimePeriodStorage;
class TimePeriodStoreFactory;

namespace base {
class Time;
}  // namespace base

namespace serp_metrics {

// SerpMetrics records and aggregates search engine usage counts.
//
// Counts are exposed for six reporting windows:
//  - Yesterday: searches from the most recent completed calendar day
//    (00:00:00 to 23:59:59 in the reporting timezone). Only searches not yet
//    reported (based on the last successful usage ping) are included.
//  - Last week: searches from the previous complete ISO week
//    (Monday 00:00:00 to Sunday 23:59:59 in the reporting timezone).
//  - Last month: searches from the previous complete calendar month
//    (first day 00:00:00 to last day 23:59:59 in the reporting timezone).
//  - Stale period: searches older than yesterday (but still within the
//    `TimePeriodStorage` retention window).
//  - Stale week period: searches older than last week's start (but still
//    within the `TimePeriodStorage` retention window).
//  - Stale month period: searches older than last month's start (but still
//    within the `TimePeriodStorage` retention window).

class SerpMetrics final {
 public:
  SerpMetrics(PrefService* local_state,
              const TimePeriodStoreFactory& time_period_store_factory);

  SerpMetrics(const SerpMetrics&) = delete;
  SerpMetrics& operator=(const SerpMetrics&) = delete;

  ~SerpMetrics();

  void RecordSearch(SerpMetricType type);
  size_t GetSearchCountForYesterday(SerpMetricType type) const;
  size_t GetSearchCountForLastWeek(SerpMetricType type) const;
  size_t GetSearchCountForLastMonth(SerpMetricType type) const;

  // Returns searches older than yesterday but within the retention window,
  // summed across all engine types.
  size_t GetSearchCountForStalePeriod() const;

  // Returns searches older than last week's start but within the retention
  // window, summed across all engine types.
  size_t GetSearchCountForStaleWeekPeriod() const;

  // Returns searches older than last month's start but within the retention
  // window, summed across all engine types.
  size_t GetSearchCountForStaleMonthPeriod() const;

  void ClearHistory();

  // Test helpers to return the total search count stored in `TimePeriodStorage`
  // without filtering by time range or staleness.
  size_t GetSearchCountForTesting(SerpMetricType type) const;

 private:
  // Returns the start of the daily stale period. Reads `kLastCheckYMD` to find
  // the last day a daily ping was sent. Returns an empty time if the date is
  // unavailable or invalid, treating the full retention window as stale.
  base::Time GetStartOfStaleDayPeriod() const;

  // Returns the start of the weekly stale period. Uses `kLastCheckWOY` as a
  // "has been sent" flag; if zero, the full retention window is stale.
  // Otherwise computes the start of the ISO week containing the `kLastCheckYMD`
  // date as the cutoff.
  base::Time GetStartOfStaleWeekPeriod() const;

  // Returns the start of the monthly stale period. Uses `kLastCheckMonth` as a
  // "has been sent" flag; if zero, the full retention window is stale.
  // Otherwise computes the start of the calendar month containing the
  // `kLastCheckYMD` date as the cutoff.
  base::Time GetStartOfStaleMonthPeriod() const;

  const raw_ptr<PrefService> local_state_;  // Not owned.

  base::flat_map<SerpMetricType, std::unique_ptr<TimePeriodStorage>>
      time_period_storages_;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_
