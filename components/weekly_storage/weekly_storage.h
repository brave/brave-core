/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_WEEKLY_STORAGE_WEEKLY_STORAGE_H_
#define BRAVE_COMPONENTS_WEEKLY_STORAGE_WEEKLY_STORAGE_H_

#include <list>
#include <memory>

#include "base/time/time.h"

namespace base {
class Clock;
}

class PrefService;

// Mostly used by various P3A recorders - allows to track a sum of some
// values added from time to time via |AddDelta| over a last week.
// Requires |pref_name| to be already registered.
// Feel free to improve and refactor it - templatize a stored value type,
// change weekly interval or make a keyed service from it.
class WeeklyStorage {
 public:
  WeeklyStorage(PrefService* prefs, const char* pref_name);

  // For tests.
  WeeklyStorage(PrefService* user_prefs,
                const char* pref_name,
                std::unique_ptr<base::Clock> clock);
  ~WeeklyStorage();

  WeeklyStorage(const WeeklyStorage&) = delete;
  WeeklyStorage& operator=(const WeeklyStorage&) = delete;

  void AddDelta(uint64_t delta);
  uint64_t GetWeeklySum() const;
  uint64_t GetHighestValueInWeek() const;
  bool IsOneWeekPassed() const;

 private:
  struct DailyValue {
    base::Time day;
    uint64_t value = 0ull;
  };
  void Load();
  void Save();

  PrefService* prefs_ = nullptr;
  const char* pref_name_ = nullptr;
  std::unique_ptr<base::Clock> clock_;

  std::list<DailyValue> daily_values_;
};

#endif  // BRAVE_COMPONENTS_WEEKLY_STORAGE_WEEKLY_STORAGE_H_
