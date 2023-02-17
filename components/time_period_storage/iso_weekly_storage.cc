/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/time_period_storage/iso_weekly_storage.h"

#include "base/time/clock.h"

namespace {
constexpr size_t kRecordPeriodDays = 14;
}  // namespace

ISOWeeklyStorage::ISOWeeklyStorage(PrefService* prefs, const char* pref_name)
    : TimePeriodStorage(prefs, pref_name, kRecordPeriodDays) {}

uint64_t ISOWeeklyStorage::GetLastISOWeekSum() const {
  return GetPeriodSumInTimeRange(GetLastMondayTime(1),
                                 GetLastMondayTime(0) - base::Days(1));
}

uint64_t ISOWeeklyStorage::GetCurrentISOWeekSum() const {
  return GetPeriodSumInTimeRange(GetLastMondayTime(0), clock_->Now());
}

base::Time ISOWeeklyStorage::GetLastMondayTime(int week_offset) const {
  base::Time midnight = clock_->Now().LocalMidnight();
  base::Time::Exploded exploded;
  midnight.LocalExplode(&exploded);

  int days_after_monday = 0;
  if (exploded.day_of_week >= 1) {
    days_after_monday = exploded.day_of_week - 1;
  } else {
    days_after_monday = 6;
  }

  return midnight - base::Days(days_after_monday + (week_offset * 7));
}
