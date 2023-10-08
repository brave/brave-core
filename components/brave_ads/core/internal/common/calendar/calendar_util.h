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

constexpr bool IsLeapYear(const int year) noexcept {
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

// Four digit year "2007", 1-based month (values 1 = January, etc.), 1-based day
// of month (1-31).
int DayOfWeek(int year, int month, int day);

int DayOfWeek(base::Time time, bool is_local);

// 1-based month (values 1 = January, etc.).
constexpr int DaysPerMonth(const int year, const int month) noexcept {
  CHECK((month >= 1 && month <= 12));

  constexpr int kDaysPerMonth[] = {
      31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31  // non leap year.
  };

  return kDaysPerMonth[month - 1] +
         static_cast<int>(month == /*february*/ 2 && IsLeapYear(year));
}

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_CALENDAR_CALENDAR_UTIL_H_
