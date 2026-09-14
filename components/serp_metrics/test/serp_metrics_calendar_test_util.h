/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_TEST_SERP_METRICS_CALENDAR_TEST_UTIL_H_
#define BRAVE_COMPONENTS_SERP_METRICS_TEST_SERP_METRICS_CALENDAR_TEST_UTIL_H_

// Calendar date helpers for SERP metrics tests.

#include "base/check.h"

namespace serp_metrics {

// Returns true if `year` is a leap year under the Gregorian calendar rule where
// a year is a leap year if divisible by 4, except for century years, which must
// be divisible by 400.
inline constexpr bool IsLeapYear(int year) {
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

// Returns the number of days in `month` for the given `year`, where `month` is
// 1-based (1 = January through 12 = December).
inline constexpr int DaysInMonth(int year, int month) {
  CHECK(month >= 1 && month <= 12);
  switch (month) {
    case 2:  // February
      return IsLeapYear(year) ? 29 : 28;
    case 4:   // April
    case 6:   // June
    case 9:   // September
    case 11:  // November
      return 30;
    default:  // January, March, May, July, August, October, December
      return 31;
  }
}

}  // namespace serp_metrics

#endif  // BRAVE_COMPONENTS_SERP_METRICS_TEST_SERP_METRICS_CALENDAR_TEST_UTIL_H_
