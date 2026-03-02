/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_TIME_PERIOD_STORAGE_H_
#define BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_TIME_PERIOD_STORAGE_H_

#include <list>
#include <memory>

#include "base/time/time.h"

namespace base {
class Clock;
}

class TimePeriodStore;

// Mostly used by various P3A recorders - allows to track a sum of some
// values added from time to time via |AddDelta| over the last predefined time
// period.
class TimePeriodStorage {
 public:
  TimePeriodStorage(std::unique_ptr<TimePeriodStore> store,
                    size_t period_days,
                    bool should_offset_dst = true);

  // This constructor is for tests and internal use only.
  TimePeriodStorage(std::unique_ptr<TimePeriodStore> store,
                    size_t period_days,
                    std::unique_ptr<base::Clock> clock,
                    bool should_offset_dst);
  ~TimePeriodStorage();

  TimePeriodStorage(const TimePeriodStorage&) = delete;
  TimePeriodStorage& operator=(const TimePeriodStorage&) = delete;

  void AddDelta(uint64_t delta);
  void SubDelta(uint64_t delta);
  void ReplaceTodaysValueIfGreater(uint64_t value);
  void ReplaceIfGreaterForDate(const base::Time& date, uint64_t value);
  uint64_t GetPeriodSumInTimeRange(const base::Time& start_time,
                                   const base::Time& end_time) const;
  uint64_t GetPeriodSum() const;
  uint64_t GetHighestValueInPeriod() const;
  bool IsOnePeriodPassed() const;

  void Clear();

 protected:
  std::unique_ptr<base::Clock> clock_;

 private:
  struct DailyValue {
    base::Time day;
    uint64_t value = 0ull;
  };
  void FilterToPeriod();
  void Load();
  void Save();

  std::unique_ptr<TimePeriodStore> store_;
  size_t period_days_;
  const bool should_offset_dst_;

  std::list<DailyValue> daily_values_;
};

#endif  // BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_TIME_PERIOD_STORAGE_H_
