/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_storage.h"

#include <numeric>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
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

base::Time Midnight(base::Time time) {
  return time.LocalMidnight();
}

// Returns the midnight that starts the calendar day after `time`. Safe
// across DST transitions and does not require `time` to be at midnight.
base::Time NextMidnight(base::Time time) {
  // No precondition is enforced on `time` being at midnight. Two legitimate
  // cases produce non-midnight inputs: legacy prefs written by the old
  // DST-offset code contain timestamps 1 hour past midnight, and travel across
  // timezones means a timestamp that was midnight in the original timezone is
  // no longer midnight in the current one. Adding `kNextMidnightOffset` always
  // lands within the next calendar day regardless of the input, so `Midnight`
  // returns its correct start.
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
  return Midnight(time + kNextMidnightOffset);
}

}  // namespace

SerpMetricsTimePeriodStorage::SerpMetricsTimePeriodStorage(
    std::unique_ptr<SerpMetricsTimePeriodStore> store,
    size_t period_days)
    : store_(std::move(store)), period_days_(period_days) {
  CHECK(store_);
  Load();
}

SerpMetricsTimePeriodStorage::~SerpMetricsTimePeriodStorage() = default;

void SerpMetricsTimePeriodStorage::AddDelta(uint64_t delta) {
  FilterToPeriod();
  daily_values_.front().value += delta;
  Save();
}

uint64_t SerpMetricsTimePeriodStorage::GetPeriodSumInTimeRange(
    const base::Time& start_time,
    const base::Time& end_time) const {
  // We only record values between the specified time range (inclusive).
  return std::accumulate(daily_values_.begin(), daily_values_.end(), 0ULL,
                         [start_time, end_time](uint64_t acc, const auto& u2) {
                           uint64_t add = 0;
                           // Check only last continious days.
                           if (u2.day >= start_time && u2.day <= end_time) {
                             add = u2.value;
                           }
                           return acc + add;
                         });
}

uint64_t SerpMetricsTimePeriodStorage::GetPeriodSum() const {
  const base::Time now = base::Time::Now();
  const base::Time n_days_ago = Midnight(now) - base::Days(period_days_ - 1);
  return GetPeriodSumInTimeRange(n_days_ago, now);
}

void SerpMetricsTimePeriodStorage::Clear() {
  daily_values_.clear();
  store_->Clear();
}

void SerpMetricsTimePeriodStorage::FilterToPeriod() {
  const base::Time now_midnight = Midnight(base::Time::Now());

  if (daily_values_.empty()) {
    // No prior data; seed the list with an empty bucket for today.
    daily_values_.push_front({now_midnight, 0});
    return;
  }

  // Use `NextMidnight` to correctly handle short calendar days at DST start.
  const base::Time last_recorded_midnight = daily_values_.front().day;
  base::Time bucket_midnight = NextMidnight(last_recorded_midnight);
  while (bucket_midnight <= now_midnight) {
    // Add an empty bucket for the missed day.
    daily_values_.push_front({bucket_midnight, 0});

    if (daily_values_.size() > period_days_) {
      // Drop the oldest bucket outside the window.
      daily_values_.pop_back();
    }

    bucket_midnight = NextMidnight(bucket_midnight);
  }
}

void SerpMetricsTimePeriodStorage::Load() {
  CHECK(daily_values_.empty());
  const base::ListValue* list = store_->Get();
  if (!list) {
    return;
  }
  for (const auto& it : *list) {
    CHECK(it.is_dict());
    const base::DictValue& dict = it.GetDict();
    auto day = dict.FindDouble("day");
    auto value = dict.FindDouble("value");
    if (!day || !value) {
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
  // TODO(iefremov): Optimize if needed.
  list.clear();
  for (const auto& u : daily_values_) {
    base::DictValue value;
    value.Set("day", u.day.InSecondsFSinceUnixEpoch());
    value.Set("value", static_cast<double>(u.value));
    list.Append(std::move(value));
  }
  store_->Set(std::move(list));
}

}  // namespace serp_metrics
