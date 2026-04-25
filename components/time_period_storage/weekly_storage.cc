/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/time_period_storage/weekly_storage.h"

namespace {
constexpr size_t kDaysInWeek = 7;
}

WeeklyStorage::WeeklyStorage(PrefService* prefs, const char* pref_name)
    : TimePeriodStorage(prefs, pref_name, kDaysInWeek) {}
WeeklyStorage::WeeklyStorage(PrefService* prefs,
                             const char* pref_name,
                             const char* dict_key)
    : TimePeriodStorage(prefs, pref_name, dict_key, kDaysInWeek) {}

uint64_t WeeklyStorage::GetWeeklySum() const {
  return GetPeriodSum();
}

uint64_t WeeklyStorage::GetHighestValueInWeek() const {
  return GetHighestValueInPeriod();
}

bool WeeklyStorage::IsOneWeekPassed() const {
  return IsOnePeriodPassed();
}
