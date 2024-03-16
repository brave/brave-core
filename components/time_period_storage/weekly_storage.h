/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_WEEKLY_STORAGE_H_
#define BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_WEEKLY_STORAGE_H_

#include "brave/components/time_period_storage/time_period_storage.h"

class PrefService;

class WeeklyStorage : public TimePeriodStorage {
 public:
  WeeklyStorage(PrefService* prefs, const char* pref_name);
  WeeklyStorage(PrefService* prefs,
                const char* pref_name,
                const char* dict_key);

  WeeklyStorage(const WeeklyStorage&) = delete;
  WeeklyStorage& operator=(const WeeklyStorage&) = delete;

  uint64_t GetWeeklySum() const;
  uint64_t GetHighestValueInWeek() const;
  bool IsOneWeekPassed() const;
};

#endif  // BRAVE_COMPONENTS_TIME_PERIOD_STORAGE_WEEKLY_STORAGE_H_
