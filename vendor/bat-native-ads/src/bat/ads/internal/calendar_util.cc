/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/calendar_util.h"

#include "base/check.h"
#include "base/time/time.h"
#include "bat/ads/internal/calendar_leap_year_util.h"

namespace ads {

int GetLastDayOfMonth(const int year, const int month) {
  DCHECK(month >= 1 && month <= 12);

  switch (month) {
    case 2: {  // February
      if (IsLeapYear(year)) {
        return 29;
      }

      return 28;
    }

    case 4:
    case 6:
    case 9:
    case 11: {  // April, June, September and November
      return 30;
    }

    default: {  // January, March, May, July, August, October and December
      return 31;
    }
  }
}

int GetDayOfWeek(int year, int month, int day) {
  DCHECK(month >= 1 && month <= 12);
  DCHECK(day >= 1 && day <= GetLastDayOfMonth(year, month));

  if (month < 3) {
    month += 12;
    year--;
  }

  return (day + (2 * month) + ((6 * (month + 1)) / 10) + year + (year / 4) -
          (year / 100) + (year / 400) + 1) %
         7;
}

int GetDayOfWeek(const base::Time time, const bool is_local) {
  base::Time::Exploded exploded;

  if (is_local) {
    time.LocalExplode(&exploded);
  } else {
    time.UTCExplode(&exploded);
  }
  DCHECK(exploded.HasValidValues());

  return exploded.day_of_week;
}

}  // namespace ads
