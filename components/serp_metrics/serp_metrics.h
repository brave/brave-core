/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_

#include <cstddef>

#include "base/memory/raw_ptr.h"
#include "brave/components/time_period_storage/time_period_storage.h"

class PrefService;

namespace base {
class Time;
}  // namespace base

namespace serp_metrics {

// SerpMetrics records and aggregates search engine usage counts.
//
// Counts are exposed for two reporting windows, based on the timestamp of the
// last successful usage ping (i.e., only searches not yet reported):
//  - Yesterday: searches from the most recent completed calendar day
//    (00:00:00 to 23:59:59 in the reporting timezone).
//  - Stale period: searches older than yesterday (but still within the
//    `TimePeriodStorage` retention window).

class SerpMetrics {
 public:
  SerpMetrics(PrefService* local_state, PrefService* prefs);

  SerpMetrics(const SerpMetrics&) = delete;
  SerpMetrics& operator=(const SerpMetrics&) = delete;

  virtual ~SerpMetrics();

  virtual void RecordBraveSearch();
  virtual size_t GetBraveSearchCountForYesterday() const;

  virtual void RecordGoogleSearch();
  virtual size_t GetGoogleSearchCountForYesterday() const;

  virtual void RecordOtherSearch();
  virtual size_t GetOtherSearchCountForYesterday() const;

  virtual size_t GetSearchCountForStalePeriod() const;

 private:
  // Returns the start of the stale period in local time, based on the last day
  // for which usage metrics were reported. Metrics recorded on or before the
  // `kLastCheckYMD` date are considered already reported, so the stale period
  // begins at local midnight on the day after that date. If the last check date
  // is unavailable or invalid, an empty time is returned to indicate that the
  // full retention period should be considered stale.
  base::Time GetStartOfStalePeriod() const;

  size_t GetBraveSearchCountForStalePeriod() const;
  size_t GetGoogleSearchCountForStalePeriod() const;
  size_t GetOtherSearchCountForStalePeriod() const;

  const raw_ptr<PrefService> local_state_;  // Not owned.
  const raw_ptr<PrefService> prefs_;        // Not owned.

  TimePeriodStorage brave_search_engine_time_period_storage_;
  TimePeriodStorage google_search_engine_time_period_storage_;
  TimePeriodStorage other_search_engine_time_period_storage_;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_
