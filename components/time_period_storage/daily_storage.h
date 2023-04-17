// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_DAILY_STORAGE_H_
#define BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_DAILY_STORAGE_H_

#include <list>
#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"

namespace base {
class Clock;
}

class PrefService;

// Allows to track a sum of some
// values added from time to time via |AddDelta| over the last 24 hours.
// Requires |pref_name| to be already registered.
// TODO(djandries): Refactor to extend TimePeriodStorage
class DailyStorage {
 public:
  DailyStorage(PrefService* prefs, const char* pref_name);

  // For tests.
  DailyStorage(PrefService* user_prefs,
               const char* pref_name,
               std::unique_ptr<base::Clock> clock);
  ~DailyStorage();

  DailyStorage(const DailyStorage&) = delete;
  DailyStorage& operator=(const DailyStorage&) = delete;

  void RecordValueNow(uint64_t delta);
  uint64_t GetLast24HourSum() const;

 private:
  struct DailyValue {
    base::Time time;
    uint64_t value = 0ull;
  };
  void FilterToDay();
  void Load();
  void Save();

  const raw_ptr<PrefService> prefs_ = nullptr;
  const char* pref_name_ = nullptr;
  std::unique_ptr<base::Clock> clock_;

  std::list<DailyValue> daily_values_;
};

#endif  // BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_DAILY_STORAGE_H_
