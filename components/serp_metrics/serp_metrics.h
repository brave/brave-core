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

class SerpMetrics {
 public:
  explicit SerpMetrics(PrefService* local_state);

  SerpMetrics(const SerpMetrics&) = delete;
  SerpMetrics& operator=(const SerpMetrics&) = delete;

  ~SerpMetrics();

  void RecordBraveSearch();
  size_t GetBraveSearchCountForYesterday() const;
  size_t GetBraveSearchCountForStalePeriod() const;

  void RecordGoogleSearch();
  size_t GetGoogleSearchCountForYesterday() const;
  size_t GetGoogleSearchCountForStalePeriod() const;

  void RecordOtherSearch();
  size_t GetOtherSearchCountForYesterday() const;
  size_t GetOtherSearchCountForStalePeriod() const;

 private:
  base::Time GetStartOfYesterday() const;
  base::Time GetEndOfYesterday() const;
  base::Time GetStartOfStalePeriod() const;
  base::Time GetEndOfStalePeriod() const;

  const raw_ptr<PrefService> locale_state_;  // Not owned.

  TimePeriodStorage brave_search_engine_time_period_storage_;
  TimePeriodStorage google_search_engine_time_period_storage_;
  TimePeriodStorage other_search_engine_time_period_storage_;
};

}  // namespace metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_SERP_METRICS_H_
