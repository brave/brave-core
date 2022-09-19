/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/time_period_storage/weekly_event_storage.h"

#include <list>
#include <memory>
#include <utility>

#include "base/json/values_util.h"
#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace {
static constexpr size_t kDaysInWeek = 7;
}

WeeklyEventStorage::WeeklyEventStorage(PrefService* prefs,
                                       const char* pref_name)
    : WeeklyEventStorage(prefs,
                         pref_name,
                         std::make_unique<base::DefaultClock>()) {}

// Accept an explicit clock so tests can manipulate the passage of time.
WeeklyEventStorage::WeeklyEventStorage(PrefService* prefs,
                                       const char* pref_name,
                                       std::unique_ptr<base::Clock> clock)
    : prefs_(prefs), pref_name_(pref_name), clock_(std::move(clock)) {
  DCHECK(prefs);
  DCHECK(pref_name);
  DCHECK(clock_);
  Load();
}

WeeklyEventStorage::~WeeklyEventStorage() = default;

void WeeklyEventStorage::Add(int value) {
  FilterToWeek();
  // Round the timestamp to the nearest day to make correlation harder.
  base::Time day = clock_->Now().LocalMidnight();
  events_.push_front({day, value});
  Save();
}

absl::optional<int> WeeklyEventStorage::GetLatest() {
  auto result = absl::optional<int>();
  if (HasEvent()) {
    // Assume the front is the most recent event.
    result = events_.front().value;
  }
  return result;
}

bool WeeklyEventStorage::HasEvent() {
  FilterToWeek();
  return !events_.empty();
}

void WeeklyEventStorage::FilterToWeek() {
  if (events_.empty()) {
    return;
  }

  // Remove all events older than a week.
  auto cutoff = clock_->Now() - base::Days(kDaysInWeek);
  events_.remove_if([cutoff](Event event) { return event.day <= cutoff; });
}

void WeeklyEventStorage::Load() {
  DCHECK(events_.empty());
  const auto& list = prefs_->GetList(pref_name_);
  for (const auto& it : list) {
    DCHECK(it.is_dict());
    const auto& item = it.GetDict();
    const auto day = base::ValueToTime(item.Find("day"));
    const auto value = item.FindInt("value");
    if (!day || !value) {
      continue;
    }
    events_.push_back({day.value(), value.value()});
  }
}

void WeeklyEventStorage::Save() {
  ListPrefUpdate update(prefs_, pref_name_);
  base::Value* list = update.Get();
  list->ClearList();
  for (const auto& u : events_) {
    base::Value::Dict value;
    value.Set("day", base::TimeToValue(u.day));
    value.Set("value", static_cast<int>(u.value));
    list->GetList().Append(std::move(value));
  }
}
