/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEEKLY_STORAGE_WEEKLY_EVENT_STORAGE_H_
#define BRAVE_COMPONENTS_WEEKLY_STORAGE_WEEKLY_EVENT_STORAGE_H_

#include <list>
#include <memory>
#include <utility>

#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// WeeklyStorage variant holding a list of events over the past week.
//
// Mostly used by various P3A recorders to report whether an event happened
// during the measurement period.
//
// New event values are recorded by calling `Add()` and are forgotten
// after approximately a week.
//
// Parametarized over an `enum class` to get some type checking for the caller.
// Requires |pref_name| to be already registered.
template <typename T>
class WeeklyEventStorage {
 public:
  WeeklyEventStorage(PrefService* prefs, const char* pref_name)
      : prefs_(prefs),
        pref_name_(pref_name),
        clock_(std::make_unique<base::DefaultClock>()) {
    DCHECK(pref_name);
    if (prefs) {
      Load();
    }
  }

  // Accept an explicit clock so tests can manipulate the passage of time.
  WeeklyEventStorage(PrefService* prefs,
                     const char* pref_name,
                     std::unique_ptr<base::Clock> clock)
      : prefs_(prefs), pref_name_(pref_name), clock_(std::move(clock)) {
    DCHECK(prefs);
    DCHECK(pref_name);
    Load();
  }

  ~WeeklyEventStorage() = default;

  WeeklyEventStorage(const WeeklyEventStorage&) = delete;
  WeeklyEventStorage& operator=(const WeeklyEventStorage&) = delete;

  void Add(T value) {
    FilterToWeek();
    // Round the timestamp to the nearest day to make correlation harder.
    base::Time day = clock_->Now().LocalMidnight();
    events_.push_front({day, value});
    Save();
  }

  absl::optional<T> GetLatest() {
    auto result = absl::optional<T>();
    if (HasEvent()) {
      // Assume the front is the most recent event.
      result = events_.front().value;
    }
    return result;
  }

  bool HasEvent() {
    FilterToWeek();
    return !events_.empty();
  }

 private:
  static constexpr size_t kDaysInWeek = 7;

  struct Event {
    base::Time day;
    T value = T(0);
  };

  PrefService* prefs_ = nullptr;
  const char* pref_name_ = nullptr;
  std::unique_ptr<base::Clock> clock_;

  std::list<Event> events_;

  void FilterToWeek() {
    if (events_.empty()) {
      return;
    }

    // Remove all events older than a week.
    auto cutoff = clock_->Now() - base::TimeDelta::FromDays(kDaysInWeek);
    events_.remove_if([cutoff](Event event) { return event.day <= cutoff; });
  }

  void Load() {
    DCHECK(events_.empty());
    const base::ListValue* list = prefs_->GetList(pref_name_);
    if (!list) {
      return;
    }
    for (auto& it : list->GetList()) {
      const base::Value* day = it.FindKey("day");
      const base::Value* value = it.FindKey("value");
      if (!day || !value || !day->is_double() || !value->is_double()) {
        continue;
      }
      events_.push_front({base::Time::FromDoubleT(day->GetDouble()),
                          {static_cast<T>(value->GetInt())}});
    }
  }

  void Save() {
    ListPrefUpdate update(prefs_, pref_name_);
    base::ListValue* list = update.Get();
    // TODO(iefremov): Optimize if needed.
    list->ClearList();
    for (const auto& u : events_) {
      base::DictionaryValue value;
      value.SetKey("day", base::Value(u.day.ToDoubleT()));
      value.SetIntKey("value", static_cast<int>(u.value));
      list->Append(std::move(value));
    }
  }
};

#endif  // BRAVE_COMPONENTS_WEEKLY_STORAGE_WEEKLY_EVENT_STORAGE_H_
