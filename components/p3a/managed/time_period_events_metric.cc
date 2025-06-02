/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/managed/time_period_events_metric.h"

#include <utility>

#include "base/json/json_value_converter.h"
#include "brave/components/p3a/pref_names.h"
#include "brave/components/time_period_storage/time_period_storage.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace p3a {

namespace {
constexpr base::TimeDelta kReportInterval = base::Days(1);
}  // namespace

TimePeriodEventsMetricDefinition::TimePeriodEventsMetricDefinition() = default;
TimePeriodEventsMetricDefinition::~TimePeriodEventsMetricDefinition() = default;

bool TimePeriodEventsMetricDefinition::Validate() const {
  return !buckets.empty() && period_days > 0 && !storage_key.empty() &&
         !histogram_name.empty();
}

TimePeriodEventsMetricDefinition::TimePeriodEventsMetricDefinition(
    TimePeriodEventsMetricDefinition&& other) = default;

TimePeriodEventsMetric::TimePeriodEventsMetric(
    TimePeriodEventsMetricDefinition definition,
    Delegate* delegate,
    std::string_view metric_name)
    : RemoteMetric(delegate, metric_name), definition_(std::move(definition)) {
  // Populate the buckets cache for easier access
  buckets_.reserve(definition_.buckets.size());
  for (const auto& bucket_ptr : definition_.buckets) {
    if (bucket_ptr) {
      buckets_.push_back(*bucket_ptr);
    }
  }
  definition_.buckets.clear();
}

TimePeriodEventsMetric::~TimePeriodEventsMetric() = default;

void TimePeriodEventsMetric::Init() {
  storage_ = delegate_->GetTimePeriodStorage(definition_.storage_key,
                                             definition_.period_days);
  Report();
}

void TimePeriodEventsMetric::HandleHistogramChange(
    std::string_view histogram_name,
    size_t sample) {
  size_t value_to_add = 1;
  if (definition_.add_histogram_value_to_storage) {
    value_to_add = sample;
  }
  if (definition_.report_max) {
    storage_->ReplaceTodaysValueIfGreater(value_to_add);
  } else {
    storage_->AddDelta(value_to_add);
  }
  Report();
}

std::vector<std::string_view> TimePeriodEventsMetric::GetSourceHistogramNames()
    const {
  return {definition_.histogram_name};
}

std::optional<std::vector<std::string_view>>
TimePeriodEventsMetric::GetStorageKeys() const {
  return std::vector<std::string_view>{definition_.storage_key};
}

void TimePeriodEventsMetric::Report() {
  size_t value = 0;
  if (definition_.report_max) {
    value = storage_->GetHighestValueInPeriod();
  } else {
    value = storage_->GetPeriodSum();
  }

  if (value < static_cast<size_t>(definition_.min_report_amount)) {
    return;
  }

  auto it = std::lower_bound(buckets_.begin(), buckets_.end(), value);
  size_t answer = std::distance(buckets_.begin(), it);

  delegate_->UpdateMetric(metric_name_, answer);

  report_timer_.Start(FROM_HERE, base::Time::Now() + kReportInterval, this,
                      &TimePeriodEventsMetric::Report);
}

void TimePeriodEventsMetricDefinition::RegisterJSONConverter(
    base::JSONValueConverter<TimePeriodEventsMetricDefinition>* converter) {
  converter->RegisterIntField("period_days",
                              &TimePeriodEventsMetricDefinition::period_days);
  converter->RegisterStringField(
      "histogram_name", &TimePeriodEventsMetricDefinition::histogram_name);
  converter->RegisterStringField(
      "storage_key", &TimePeriodEventsMetricDefinition::storage_key);
  converter->RegisterRepeatedInt("buckets",
                                 &TimePeriodEventsMetricDefinition::buckets);
  converter->RegisterBoolField("report_max",
                               &TimePeriodEventsMetricDefinition::report_max);
  converter->RegisterBoolField(
      "add_histogram_value_to_storage",
      &TimePeriodEventsMetricDefinition::add_histogram_value_to_storage);
  converter->RegisterIntField(
      "min_report_amount",
      &TimePeriodEventsMetricDefinition::min_report_amount);
}

}  // namespace p3a
