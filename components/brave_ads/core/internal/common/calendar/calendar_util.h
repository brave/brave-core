/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CALENDAR_CALENDAR_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CALENDAR_CALENDAR_UTIL_H_

#include "base/check.h"

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

// Returns true if `year` is a leap year.
constexpr bool IsLeapYear(int year) {
  // Every 4th year is a leap year; every 100th is not; every 400th is.
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

// Returns the day of the week, where Sunday is 0 and Saturday is 6, matching
// `struct tm`.
int DayOfWeek(base::Time time, bool is_local);

// Returns the number of days in `month` for `year`, accounting for leap years.
// Expects a 1-based month (1 = January, ..., 12 = December).
constexpr int DaysInMonth(int year, int month) {
  CHECK((month >= 1 && month <= 12));

  switch (month) {
    case 2:  // February: short by design, occasionally less so.
      return IsLeapYear(year) ? 29 : 28;
    case 4:   // April
    case 6:   // June
    case 9:   // September
    case 11:  // November
      return 30;
    default:  // January, March, May, July, August, October, December.
      return 31;
  }
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CALENDAR_CALENDAR_UTIL_H_
