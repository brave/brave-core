/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/time_period_storage/time_period_storage.h"

#include <algorithm>
#include <numeric>
#include <utility>

#include "base/ranges/algorithm.h"
#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

TimePeriodStorage::TimePeriodStorage(PrefService* prefs,
                                     const char* pref_name,
                                     size_t period_days)
    : prefs_(prefs),
      pref_name_(pref_name),
      period_days_(period_days),
      clock_(std::make_unique<base::DefaultClock>()) {
  DCHECK(pref_name);
  if (prefs) {
    Load();
  }
}

TimePeriodStorage::TimePeriodStorage(PrefService* prefs,
                                     const char* pref_name,
                                     size_t period_days,
                                     std::unique_ptr<base::Clock> clock)
    : prefs_(prefs),
      pref_name_(pref_name),
      period_days_(period_days),
      clock_(std::move(clock)) {
  DCHECK(prefs);
  DCHECK(pref_name);
  Load();
}

TimePeriodStorage::~TimePeriodStorage() = default;

void TimePeriodStorage::AddDelta(uint64_t delta) {
  FilterToPeriod();
  daily_values_.front().value += delta;
  Save();
}

void TimePeriodStorage::SubDelta(uint64_t delta) {
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

void TimePeriodStorage::ReplaceTodaysValueIfGreater(uint64_t value) {
  FilterToPeriod();
  DailyValue& today = daily_values_.front();
  if (today.value < value) {
    today.value = value;
  }
  Save();
}

void TimePeriodStorage::ReplaceIfGreaterForDate(const base::Time& date,
                                                uint64_t value) {
  FilterToPeriod();
  base::Time date_mn = date.LocalMidnight();
  std::list<DailyValue>::iterator day_insert_it = base::ranges::find_if(
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

uint64_t TimePeriodStorage::GetPeriodSum() const {
  // We record only value for last N days.
  const base::Time n_days_ago = clock_->Now() - base::Days(period_days_);
  return std::accumulate(daily_values_.begin(), daily_values_.end(), 0ull,
                         [n_days_ago](const uint64_t acc, const auto& u2) {
                           uint64_t add = 0;
                           // Check only last continious days.
                           if (u2.day > n_days_ago) {
                             add = u2.value;
                           }
                           return acc + add;
                         });
}

uint64_t TimePeriodStorage::GetHighestValueInPeriod() const {
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

bool TimePeriodStorage::IsOnePeriodPassed() const {
  // TODO(iefremov): This is not true 100% (if the browser was launched once
  // per the time period just after installation, for example).
  return daily_values_.size() == period_days_;
}

void TimePeriodStorage::FilterToPeriod() {
  base::Time now_midnight = clock_->Now().LocalMidnight();
  base::Time last_saved_midnight;

  if (!daily_values_.empty()) {
    last_saved_midnight = daily_values_.front().day;
  }

  if (now_midnight - last_saved_midnight > base::TimeDelta()) {
    // Day changed. Since we consider only small incoming intervals, lets just
    // save it with a new timestamp.
    daily_values_.push_front({now_midnight, 0});
    if (daily_values_.size() > period_days_) {
      daily_values_.pop_back();
    }
  }
}

void TimePeriodStorage::Load() {
  DCHECK(daily_values_.empty());
  const auto& list = prefs_->GetList(pref_name_);
  for (const auto& it : list) {
    DCHECK(it.is_dict());
    const base::Value::Dict& dict = it.GetDict();
    auto day = dict.FindDouble("day");
    auto value = dict.FindDouble("value");
    if (!day || !value) {
      continue;
    }
    if (daily_values_.size() == period_days_) {
      break;
    }
    daily_values_.push_back(
        {base::Time::FromDoubleT(*day), static_cast<uint64_t>(*value)});
  }
}

void TimePeriodStorage::Save() {
  DCHECK(!daily_values_.empty());
  DCHECK_LE(daily_values_.size(), period_days_);

  ListPrefUpdate update(prefs_, pref_name_);
  base::Value::List& list = update.Get()->GetList();
  // TODO(iefremov): Optimize if needed.
  list.clear();
  for (const auto& u : daily_values_) {
    base::Value::Dict value;
    value.Set("day", u.day.ToDoubleT());
    value.Set("value", static_cast<double>(u.value));
    list.Append(std::move(value));
  }
}
