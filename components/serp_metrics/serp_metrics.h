/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_
#define BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_

#include <cstddef>

#include "base/memory/raw_ptr.h"
#include "brave/components/time_period_storage/time_period_storage.h"

namespace base {
class Time;
}  // namespace base

namespace metrics {

// SerpMetrics records and reports search engine usage counts for reporting.
//
// Counts are exposed for two reporting windows, based on the timestamp of the
// last successful usage ping (i.e., only searches not yet reported):
//  - Yesterday: searches from the most recent completed calendar day
//    (00:00:00–23:59:59 in the reporting timezone).
//  - Stale period: searches older than yesterday (but still within the
//    `TimePeriodStorage` retention window).

class SerpMetrics {
 public:
  explicit SerpMetrics(PrefService* local_state);

  SerpMetrics(const SerpMetrics&) = delete;
  SerpMetrics& operator=(const SerpMetrics&) = delete;

  virtual ~SerpMetrics();

  virtual void RecordBraveSearch();
  virtual size_t GetBraveSearchCountForYesterday() const;
  virtual size_t GetBraveSearchCountForStalePeriod() const;

  virtual void RecordGoogleSearch();
  virtual size_t GetGoogleSearchCountForYesterday() const;
  virtual size_t GetGoogleSearchCountForStalePeriod() const;

  virtual void RecordOtherSearch();
  virtual size_t GetOtherSearchCountForYesterday() const;
  virtual size_t GetOtherSearchCountForStalePeriod() const;

 private:
  base::Time GetStartOfStalePeriod() const;

  const raw_ptr<PrefService> local_state_;  // Not owned.

  TimePeriodStorage brave_search_engine_time_period_storage_;
  TimePeriodStorage google_search_engine_time_period_storage_;
  TimePeriodStorage other_search_engine_time_period_storage_;
};

}  // namespace metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_
