/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/time_period_storage/weekly_event_storage.h"

#include <list>
#include <optional>

#include "base/check.h"
#include "base/json/values_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace {
static constexpr size_t kDaysInWeek = 7;
}

WeeklyEventStorage::WeeklyEventStorage(PrefService* prefs,
                                       const char* pref_name)
    : prefs_(*prefs), pref_name_(pref_name) {
  DCHECK(pref_name);
  Load();
}

WeeklyEventStorage::~WeeklyEventStorage() = default;

void WeeklyEventStorage::Add(int value) {
  FilterToWeek();
  // Round the timestamp to the nearest day to make correlation harder.
  base::Time day = base::Time::Now().LocalMidnight();
  events_.push_front({day, value});
  Save();
}

std::optional<int> WeeklyEventStorage::GetLatest() {
  auto result = std::optional<int>();
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
  const auto cutoff = base::Time::Now() - base::Days(kDaysInWeek);
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
  base::ListValue list;
  for (const auto& u : events_) {
    base::DictValue value;
    value.Set("day", base::TimeToValue(u.day));
    value.Set("value", static_cast<int>(u.value));
    list.Append(std::move(value));
  }
  prefs_->SetList(pref_name_, std::move(list));
}
