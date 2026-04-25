/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_storage.h"

#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/circular_deque.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_store.h"

namespace serp_metrics {

namespace {

// 24 hours covers a standard day; the extra 2 hours covers the maximum DST
// shift (Antarctica/Troll), guaranteeing the result lands inside the next
// calendar day without overshooting into the day after.
constexpr base::TimeDelta kNextMidnightOffset =
    base::Hours(24) + base::Hours(2);

constexpr std::string_view kDayKey = "day";
constexpr std::string_view kValueKey = "value";

// Returns the midnight that starts the calendar day after `time`. Safe
// across DST transitions and does not require `time` to be at midnight.
base::Time NextMidnight(base::Time time) {
  // No precondition is enforced on `time` being at midnight. Two legitimate
  // cases produce non-midnight inputs: legacy prefs written by the old
  // DST-offset code contain timestamps 1 hour past midnight, and travel across
  // timezones means a timestamp that was midnight in the original timezone is
  // no longer midnight in the current one. Adding `kNextMidnightOffset` always
  // lands within the next calendar day regardless of the input, so
  // `LocalMidnight` returns its correct start.
  //
  // Known limitation: if `time` appears later than 22:00 in the current local
  // timezone (possible when crossing more than 22 timezone hours, e.g. UTC+12
  // to UTC-11), `time + kNextMidnightOffset` overshoots into the day after next
  // and one empty bucket is silently skipped. A correct fix requires ICU
  // calendar arithmetic to advance by exactly one calendar day. DST transitions
  // shift clocks by at most 2 hours, so the 22:00 bound is never reached for
  // DST and all DST cases are handled correctly. Migrating all
  // `SerpMetricsTimePeriodStorage` callers to UTC buckets would eliminate both
  // the DST and timezone-travel edge cases entirely.
  return (time + kNextMidnightOffset).LocalMidnight();
}

}  // namespace

SerpMetricsTimePeriodStorage::SerpMetricsTimePeriodStorage(
    std::unique_ptr<SerpMetricsTimePeriodStore> store,
    size_t period_days)
    : time_period_store_(std::move(store)), period_days_(period_days) {
  CHECK(time_period_store_);

  Load();
}

SerpMetricsTimePeriodStorage::~SerpMetricsTimePeriodStorage() = default;

void SerpMetricsTimePeriodStorage::AddCount(uint64_t count) {
  FillToToday();
  daily_values_.front().value += count;
  Save();
}

uint64_t SerpMetricsTimePeriodStorage::GetCountForTimeRange(
    const base::Time& start_time,
    const base::Time& end_time) const {
  uint64_t count = 0;
  for (const auto& daily_value : daily_values_) {
    if (daily_value.time >= start_time && daily_value.time <= end_time) {
      count += daily_value.value;
    }
  }
  return count;
}

uint64_t SerpMetricsTimePeriodStorage::GetCount() const {
  const base::Time now = base::Time::Now();
  const base::Time n_days_ago =
      now.LocalMidnight() - base::Days(period_days_ - 1);
  return GetCountForTimeRange(n_days_ago, now);
}

void SerpMetricsTimePeriodStorage::Clear() {
  daily_values_.clear();
  time_period_store_->Clear();
}

void SerpMetricsTimePeriodStorage::FillToToday() {
  const base::Time now_midnight = base::Time::Now().LocalMidnight();

  if (daily_values_.empty()) {
    // No prior data; seed with an empty bucket for today.
    daily_values_.push_front({now_midnight, 0});
    return;
  }

  // Use `NextMidnight` to correctly handle short calendar days at DST start.
  const base::Time last_recorded_midnight = daily_values_.front().time;
  base::Time midnight_bucket = NextMidnight(last_recorded_midnight);
  while (midnight_bucket <= now_midnight) {
    daily_values_.push_front({midnight_bucket, 0});
    if (daily_values_.size() > period_days_) {
      daily_values_.pop_back();
    }
    midnight_bucket = NextMidnight(midnight_bucket);
  }
}

void SerpMetricsTimePeriodStorage::Load() {
  CHECK(daily_values_.empty());

  const base::ListValue* const list = time_period_store_->Get();
  if (!list) {
    return;
  }

  for (const auto& daily_value : *list) {
    if (!daily_value.is_dict()) {
      continue;
    }
    const base::DictValue& dict = daily_value.GetDict();

    std::optional<double> day = dict.FindDouble(kDayKey);
    if (!day) {
      continue;
    }

    std::optional<double> value = dict.FindDouble(kValueKey);
    if (!value) {
      continue;
    }

    if (daily_values_.size() == period_days_) {
      break;
    }

    daily_values_.push_back({base::Time::FromSecondsSinceUnixEpoch(*day),
                             static_cast<uint64_t>(*value)});
  }
}

void SerpMetricsTimePeriodStorage::Save() {
  CHECK(!daily_values_.empty());
  CHECK_LE(daily_values_.size(), period_days_);

  base::ListValue list;
  for (const auto& daily_value : daily_values_) {
    list.Append(base::DictValue()
                    .Set(kDayKey, daily_value.time.InSecondsFSinceUnixEpoch())
                    .Set(kValueKey, static_cast<double>(daily_value.value)));
  }
  time_period_store_->Set(std::move(list));
}

}  // namespace serp_metrics
