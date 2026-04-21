/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_METRICS_TIME_PERIOD_STORAGE_H_
#define BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_METRICS_TIME_PERIOD_STORAGE_H_

#include <list>
#include <memory>

#include "base/time/time.h"

namespace serp_metrics {

class SerpMetricsTimePeriodStore;

// Allows to track a sum of some values added from time to time via `AddDelta`
// over the last predefined time period.
class SerpMetricsTimePeriodStorage {
 public:
  SerpMetricsTimePeriodStorage(
      std::unique_ptr<SerpMetricsTimePeriodStore> store,
      size_t period_days);

  SerpMetricsTimePeriodStorage(const SerpMetricsTimePeriodStorage&) = delete;
  SerpMetricsTimePeriodStorage& operator=(const SerpMetricsTimePeriodStorage&) =
      delete;

  ~SerpMetricsTimePeriodStorage();

  void AddDelta(uint64_t delta);
  uint64_t GetPeriodSumInTimeRange(const base::Time& start_time,
                                   const base::Time& end_time) const;
  uint64_t GetPeriodSum() const;

  void Clear();

 private:
  struct DailyValue {
    base::Time day;
    uint64_t value = 0ULL;
  };
  void FilterToPeriod();
  void Load();
  void Save();

  std::unique_ptr<SerpMetricsTimePeriodStore> store_;
  size_t period_days_;

  std::list<DailyValue> daily_values_;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_METRICS_TIME_PERIOD_STORAGE_H_
