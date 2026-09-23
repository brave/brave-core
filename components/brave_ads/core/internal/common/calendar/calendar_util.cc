/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/calendar/calendar_util.h"

#include "base/check.h"

namespace brave_ads {

namespace {

// Returns true if `year` is a leap year.
bool IsLeapYear(int year) {
  // Every 4th year is a leap year; every 100th is not; every 400th is.
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

}  // namespace

int DaysInMonth(int year, int month) {
  CHECK(month >= 1 && month <= 12);

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
