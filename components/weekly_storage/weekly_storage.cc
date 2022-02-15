/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/weekly_storage/weekly_storage.h"

#include <numeric>
#include <utility>

#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace {
constexpr size_t kDaysInWeek = 7;
}

WeeklyStorage::WeeklyStorage(PrefService* prefs, const char* pref_name)
    : prefs_(prefs),
      pref_name_(pref_name),
      clock_(std::make_unique<base::DefaultClock>()) {
  DCHECK(pref_name);
  if (prefs) {
    Load();
  }
}

WeeklyStorage::WeeklyStorage(PrefService* prefs,
                             const char* pref_name,
                             std::unique_ptr<base::Clock> clock)
    : prefs_(prefs), pref_name_(pref_name), clock_(std::move(clock)) {
  DCHECK(prefs);
  DCHECK(pref_name);
  Load();
}

WeeklyStorage::~WeeklyStorage() = default;

void WeeklyStorage::AddDelta(uint64_t delta) {
  FilterToWeek();
  daily_values_.front().value += delta;
  Save();
}

void WeeklyStorage::ReplaceTodaysValueIfGreater(uint64_t value) {
  FilterToWeek();
  DailyValue& today = daily_values_.front();
  if (today.value < value) {
    today.value = value;
  }
  Save();
}

uint64_t WeeklyStorage::GetWeeklySum() const {
  // We record only value for last N days.
  const base::Time n_days_ago = clock_->Now() - base::Days(kDaysInWeek);
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

uint64_t WeeklyStorage::GetHighestValueInWeek() const {
  // We record only value for last N days.
  const base::Time n_days_ago = clock_->Now() - base::Days(kDaysInWeek);
  std::list<DailyValue> last_weeks_daily_values(daily_values_.size());
  auto copied_it =
      std::copy_if(daily_values_.begin(), daily_values_.end(),
                   last_weeks_daily_values.begin(),
                   [n_days_ago](auto i) { return i.day > n_days_ago; });
  last_weeks_daily_values.resize(
      std::distance(last_weeks_daily_values.begin(), copied_it));

  auto highest_it = std::max_element(last_weeks_daily_values.begin(),
                                     last_weeks_daily_values.end(),
                                     [](const auto& left, const auto& right) {
                                       return left.value < right.value;
                                     });
  if (highest_it == last_weeks_daily_values.end()) {
    return 0;
  }
  return highest_it->value;
}

bool WeeklyStorage::IsOneWeekPassed() const {
  // TODO(iefremov): This is not true 100% (if the browser was launched once
  // per week just after installation, for example).
  return daily_values_.size() == kDaysInWeek;
}

void WeeklyStorage::FilterToWeek() {
  base::Time now_midnight = clock_->Now().LocalMidnight();
  base::Time last_saved_midnight;

  if (!daily_values_.empty()) {
    last_saved_midnight = daily_values_.front().day;
  }

  if (now_midnight - last_saved_midnight > base::TimeDelta()) {
    // Day changed. Since we consider only small incoming intervals, lets just
    // save it with a new timestamp.
    daily_values_.push_front({now_midnight, 0});
    if (daily_values_.size() > kDaysInWeek) {
      daily_values_.pop_back();
    }
  }
}

void WeeklyStorage::Load() {
  DCHECK(daily_values_.empty());
  const base::Value* list = prefs_->GetList(pref_name_);
  if (!list) {
    return;
  }
  for (auto& it : list->GetListDeprecated()) {
    const base::Value* day = it.FindKey("day");
    const base::Value* value = it.FindKey("value");
    if (!day || !value || !day->is_double() || !value->is_double()) {
      continue;
    }
    if (daily_values_.size() == kDaysInWeek) {
      break;
    }
    daily_values_.push_back({base::Time::FromDoubleT(day->GetDouble()),
                             static_cast<uint64_t>(value->GetDouble())});
  }
}

void WeeklyStorage::Save() {
  DCHECK(!daily_values_.empty());
  DCHECK_LE(daily_values_.size(), kDaysInWeek);

  ListPrefUpdate update(prefs_, pref_name_);
  base::Value* list = update.Get();
  // TODO(iefremov): Optimize if needed.
  list->ClearList();
  for (const auto& u : daily_values_) {
    base::DictionaryValue value;
    value.SetKey("day", base::Value(u.day.ToDoubleT()));
    value.SetDoubleKey("value", u.value);
    list->Append(std::move(value));
  }
}
