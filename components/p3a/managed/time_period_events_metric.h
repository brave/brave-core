/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_MANAGED_TIME_PERIOD_EVENTS_METRIC_H_
#define BRAVE_COMPONENTS_P3A_MANAGED_TIME_PERIOD_EVENTS_METRIC_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/json/json_value_converter.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/p3a/managed/remote_metric.h"
#include "brave/components/time_period_storage/time_period_storage.h"

class PrefService;

namespace p3a {

struct TimePeriodEventsMetricDefinition {
  TimePeriodEventsMetricDefinition();
  ~TimePeriodEventsMetricDefinition();

  TimePeriodEventsMetricDefinition(const TimePeriodEventsMetricDefinition&) =
      delete;
  TimePeriodEventsMetricDefinition& operator=(
      const TimePeriodEventsMetricDefinition&) = delete;
  TimePeriodEventsMetricDefinition(TimePeriodEventsMetricDefinition&& other);

  static void RegisterJSONConverter(
      base::JSONValueConverter<TimePeriodEventsMetricDefinition>* converter);

  bool Validate() const;
  // Name of the histogram to observe for events
  std::string histogram_name;
  // Unique key used to store metric data in preferences
  std::string storage_key;
  // Number of days to track events for each reporting period
  int period_days = 0;
  // Exclusive max bucket boundaries for categorizing event counts
  std::vector<std::unique_ptr<int>> buckets;
  // If true, reports the maximum value in period instead of sum
  bool report_max = false;
  // If true, adds the histogram sample value instead of just counting events
  bool add_histogram_value_to_storage = false;
  // Minimum value to report, used to establish a floor for reporting
  int min_report_amount = 0;
};

class TimePeriodEventsMetric : public RemoteMetric {
 public:
  TimePeriodEventsMetric(PrefService* local_state,
                         TimePeriodEventsMetricDefinition definition,
                         base::RepeatingCallback<void(size_t)> update_callback);
  ~TimePeriodEventsMetric() override;

  TimePeriodEventsMetric(const TimePeriodEventsMetric&) = delete;
  TimePeriodEventsMetric& operator=(const TimePeriodEventsMetric&) = delete;

  void HandleHistogramChange(std::string_view histogram_name,
                             size_t sample) override;

  std::vector<std::string_view> GetSourceHistogramNames() const override;

  std::optional<std::string_view> GetStorageKey() const override;

 private:
  void Report();

  raw_ptr<PrefService> local_state_;
  TimePeriodEventsMetricDefinition definition_;
  TimePeriodStorage storage_;
  base::RepeatingCallback<void(size_t)> update_callback_;

  base::WallClockTimer report_timer_;
  // Cache of int values from definition_.buckets for easier access
  std::vector<size_t> buckets_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_MANAGED_TIME_PERIOD_EVENTS_METRIC_H_
