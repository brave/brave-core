/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_METRICS_TIME_PERIOD_STORAGE_H_
#define BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_METRICS_TIME_PERIOD_STORAGE_H_

#include <memory>

#include "base/containers/circular_deque.h"
#include "base/time/time.h"
#include "brave/components/serp_metrics/time_period_storage/serp_time_period_storage_daily_value_info.h"

namespace serp_metrics {

class SerpMetricsTimePeriodStore;

// Allows to track a sum of some values added from time to time via `AddCount`
// over the last predefined time period.
class SerpMetricsTimePeriodStorage final {
 public:
  SerpMetricsTimePeriodStorage(
      std::unique_ptr<SerpMetricsTimePeriodStore> store,
      size_t period_days);

  SerpMetricsTimePeriodStorage(const SerpMetricsTimePeriodStorage&) = delete;
  SerpMetricsTimePeriodStorage& operator=(const SerpMetricsTimePeriodStorage&) =
      delete;

  ~SerpMetricsTimePeriodStorage();

  void AddCount(uint64_t count);
  uint64_t GetCountForTimeRange(const base::Time& start_time,
                                const base::Time& end_time) const;
  uint64_t GetCount() const;

  void Clear();

 private:
  void FillToToday();
  void Load();
  void Save();

  std::unique_ptr<SerpMetricsTimePeriodStore> time_period_store_;
  size_t period_days_;

  base::circular_deque<DailyValueInfo> daily_values_;
};

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_TIME_PERIOD_STORAGE_SERP_METRICS_TIME_PERIOD_STORAGE_H_
