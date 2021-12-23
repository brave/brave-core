// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/weekly_storage/daily_storage.h"

#include <numeric>
#include <utility>

#include "base/logging.h"
#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

DailyStorage::DailyStorage(PrefService* prefs, const char* pref_name)
    : prefs_(prefs),
      pref_name_(pref_name),
      clock_(std::make_unique<base::DefaultClock>()) {
  DCHECK(pref_name);
  if (prefs) {
    Load();
  }
}

DailyStorage::DailyStorage(PrefService* prefs,
                           const char* pref_name,
                           std::unique_ptr<base::Clock> clock)
    : prefs_(prefs), pref_name_(pref_name), clock_(std::move(clock)) {
  DCHECK(prefs);
  DCHECK(pref_name);
  Load();
}

DailyStorage::~DailyStorage() = default;

void DailyStorage::RecordValueNow(uint64_t delta) {
  daily_values_.push_front({clock_->Now(), delta});
  Save();
}

uint64_t DailyStorage::GetLast24HourSum() const {
  // We record only value for last N days.
  return std::accumulate(daily_values_.begin(), daily_values_.end(), 0ull,
                         [](const uint64_t acc, const DailyValue& item) {
                           return acc + item.value;
                         });
}

void DailyStorage::FilterToDay() {
  if (daily_values_.empty()) {
    return;
  }
  // Remove all values that aren't within the last 24 hours
  base::Time min = clock_->Now() - base::Days(1);
  daily_values_.remove_if([min](DailyValue val) { return (val.time <= min); });
}

void DailyStorage::Load() {
  DCHECK(daily_values_.empty());
  const base::Value* list = prefs_->GetList(pref_name_);
  if (!list) {
    return;
  }
  base::Time min = clock_->Now() - base::Days(1);
  for (const auto& it : list->GetList()) {
    const base::Value* day = it.FindKey("day");
    const base::Value* value = it.FindKey("value");
    // Validate correct data format
    if (!day || !value || !day->is_double() || !value->is_double()) {
      continue;
    }
    // Disregard if old value
    auto time = base::Time::FromDoubleT(day->GetDouble());
    if (time <= min) {
      continue;
    }
    daily_values_.push_back({time, static_cast<uint64_t>(value->GetDouble())});
  }
}

void DailyStorage::Save() {
  FilterToDay();
  ListPrefUpdate update(prefs_, pref_name_);
  base::ListValue* list = update.Get();
  list->ClearList();
  for (const auto& u : daily_values_) {
    base::DictionaryValue value;
    value.SetKey("day", base::Value(u.time.ToDoubleT()));
    value.SetDoubleKey("value", u.value);
    list->Append(std::move(value));
  }
}
