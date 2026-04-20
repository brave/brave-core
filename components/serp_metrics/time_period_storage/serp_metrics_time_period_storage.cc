/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_storage.h"

#include <algorithm>
#include <numeric>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_pref_time_period_store.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_store.h"
#include "components/prefs/pref_service.h"

namespace serp_metrics {

namespace {

// Used to compensate for DST-related differences. i.e. time
// method arguments not matching up with stored time values.
constexpr base::TimeDelta kPotentialDSTOffset = base::Hours(1);

// Used by `NextMidnight` when `should_use_fixed_day` is `false`. 24 hours
// covers a standard day; the extra 2 hours covers the maximum DST shift
// (Antarctica/Troll), guaranteeing the result lands inside the next calendar
// day without overshooting into the day after.
constexpr base::TimeDelta kNextMidnightOffset =
    base::Hours(24) + base::Hours(2);

}  // namespace

SerpMetricsTimePeriodStorage::SerpMetricsTimePeriodStorage(
    std::unique_ptr<SerpMetricsTimePeriodStore> store,
    size_t period_days,
    bool should_use_utc,
    bool should_offset_dst)
    : clock_(std::make_unique<base::DefaultClock>()),
      store_(std::move(store)),
      period_days_(period_days),
      should_use_utc_(should_use_utc),
      should_offset_dst_(should_offset_dst) {
  CHECK(store_);
  Load();
}

SerpMetricsTimePeriodStorage::SerpMetricsTimePeriodStorage(
    PrefService* prefs,
    const char* pref_name,
    size_t period_days,
    bool should_use_utc,
    bool should_offset_dst)
    : SerpMetricsTimePeriodStorage(prefs,
                                   pref_name,
                                   nullptr,
                                   period_days,
                                   should_use_utc,
                                   should_offset_dst) {}

SerpMetricsTimePeriodStorage::SerpMetricsTimePeriodStorage(
    PrefService* prefs,
    const char* pref_name,
    const char* dict_key,
    size_t period_days,
    bool should_use_utc,
    bool should_offset_dst)
    : clock_(std::make_unique<base::DefaultClock>()),
      store_(std::make_unique<SerpMetricsPrefTimePeriodStore>(prefs,
                                                              pref_name,
                                                              dict_key)),
      period_days_(period_days),
      should_use_utc_(should_use_utc),
      should_offset_dst_(should_offset_dst) {
  CHECK(pref_name);
  if (prefs) {
    Load();
  }
}

SerpMetricsTimePeriodStorage::SerpMetricsTimePeriodStorage(
    PrefService* prefs,
    const char* pref_name,
    const char* dict_key,
    size_t period_days,
    std::unique_ptr<base::Clock> clock,
    bool should_use_utc,
    bool should_offset_dst)
    : clock_(std::move(clock)),
      store_(std::make_unique<SerpMetricsPrefTimePeriodStore>(prefs,
                                                              pref_name,
                                                              dict_key)),
      period_days_(period_days),
      should_use_utc_(should_use_utc),
      should_offset_dst_(should_offset_dst) {
  CHECK(prefs);
  CHECK(pref_name);
  Load();
}

SerpMetricsTimePeriodStorage::~SerpMetricsTimePeriodStorage() = default;

void SerpMetricsTimePeriodStorage::AddDelta(uint64_t delta) {
  FilterToPeriod();
  daily_values_.front().value += delta;
  Save();
}

void SerpMetricsTimePeriodStorage::SubDelta(uint64_t delta) {
  FilterToPeriod();
  for (DailyValue& daily_value : daily_values_) {
    if (delta == 0) {
      break;
    }
    uint64_t day_delta = std::min(daily_value.value, delta);
    daily_value.value -= day_delta;
    delta -= day_delta;
  }
  Save();
}

void SerpMetricsTimePeriodStorage::ReplaceTodaysValueIfGreater(uint64_t value) {
  FilterToPeriod();
  DailyValue& today = daily_values_.front();
  if (today.value < value) {
    today.value = value;
  }
  Save();
}

void SerpMetricsTimePeriodStorage::ReplaceIfGreaterForDate(
    const base::Time& date,
    uint64_t value) {
  FilterToPeriod();
  base::Time date_mn = Midnight(date);
  std::list<DailyValue>::iterator day_insert_it = std::ranges::find_if(
      daily_values_,
      [date_mn](const DailyValue& val) { return val.day <= date_mn; });
  if (day_insert_it != daily_values_.end() && day_insert_it->day == date_mn) {
    // update daily value if it exists for date
    if (value > day_insert_it->value) {
      day_insert_it->value = value;
    }
  } else {
    daily_values_.insert(day_insert_it, {date_mn, value});
  }
  Save();
}

uint64_t SerpMetricsTimePeriodStorage::GetPeriodSumInTimeRange(
    const base::Time& start_time,
    const base::Time& end_time) const {
  const base::TimeDelta dst_offset = GetDstOffset();
  // We only record values between the specified time range (inclusive).
  return std::accumulate(
      daily_values_.begin(), daily_values_.end(), 0ull,
      [start_time, end_time, dst_offset](uint64_t acc, const auto& u2) {
        uint64_t add = 0;
        // Check only last continious days.
        if (u2.day >= start_time - dst_offset &&
            u2.day <= end_time + dst_offset) {
          add = u2.value;
        }
        return acc + add;
      });
}

uint64_t SerpMetricsTimePeriodStorage::GetPeriodSum() const {
  const base::Time now = clock_->Now();
  const base::Time n_days_ago = Midnight(now) - base::Days(period_days_ - 1);
  return GetPeriodSumInTimeRange(n_days_ago, now);
}

uint64_t SerpMetricsTimePeriodStorage::GetHighestValueInPeriod() const {
  // We record only value for last N days.
  const base::Time n_days_ago = clock_->Now() - base::Days(period_days_);
  std::list<DailyValue> in_period_daily_values(daily_values_.size());
  auto copied_it =
      std::copy_if(daily_values_.begin(), daily_values_.end(),
                   in_period_daily_values.begin(),
                   [n_days_ago](auto i) { return i.day > n_days_ago; });
  in_period_daily_values.resize(
      std::distance(in_period_daily_values.begin(), copied_it));

  auto highest_it = std::max_element(in_period_daily_values.begin(),
                                     in_period_daily_values.end(),
                                     [](const auto& left, const auto& right) {
                                       return left.value < right.value;
                                     });
  if (highest_it == in_period_daily_values.end()) {
    return 0;
  }
  return highest_it->value;
}

bool SerpMetricsTimePeriodStorage::IsOnePeriodPassed() const {
  return daily_values_.size() == period_days_;
}

void SerpMetricsTimePeriodStorage::Clear() {
  daily_values_.clear();
  store_->Clear();
}

base::Time SerpMetricsTimePeriodStorage::Midnight(base::Time time) const {
  return should_use_utc_ ? time.UTCMidnight() : time.LocalMidnight();
}

base::Time SerpMetricsTimePeriodStorage::NextMidnight(base::Time time) const {
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

base::TimeDelta SerpMetricsTimePeriodStorage::GetDstOffset() const {
  // DST offset is only applied when using local time, since UTC time does not
  // have DST adjustments.
  if (should_use_utc_ || !should_offset_dst_) {
    return base::TimeDelta();
  }

  return kPotentialDSTOffset;
}

void SerpMetricsTimePeriodStorage::FilterToPeriod() {
  const base::Time now_midnight = Midnight(clock_->Now());

  if (daily_values_.empty()) {
    // No prior data; seed the list with an empty bucket for today.
    daily_values_.push_front({now_midnight, 0});
    return;
  }

  // When DST offsetting is disabled and local time is used (SERP), use
  // `NextMidnight` to correctly handle short calendar days at DST start.
  // For P3A, `GetDstOffset` extends the loop condition by one hour to cover
  // a potential DST shift. For UTC, days are always exactly 24 hours.
  const bool should_use_fixed_day = should_offset_dst_ || should_use_utc_;
  const base::Time last_recorded_midnight = daily_values_.front().day;
  base::Time bucket_midnight = should_use_fixed_day
                                   ? last_recorded_midnight + base::Days(1)
                                   : NextMidnight(last_recorded_midnight);
  while (bucket_midnight <= now_midnight + GetDstOffset()) {
    // Add an empty bucket for the missed day.
    daily_values_.push_front({bucket_midnight, 0});

    if (daily_values_.size() > period_days_) {
      // Drop the oldest bucket outside the window.
      daily_values_.pop_back();
    }

    // Advance to the next bucket.
    bucket_midnight = should_use_fixed_day ? bucket_midnight + base::Days(1)
                                           : NextMidnight(bucket_midnight);
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
