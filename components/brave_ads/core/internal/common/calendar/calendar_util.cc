/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/calendar/calendar_util.h"

#include "base/time/time.h"

namespace brave_ads {

int DayOfWeek(int year, int month, int day) {
  CHECK((month >= 1 && month <= 12));
  CHECK((day >= 1 && day <= DaysPerMonth(year, month)));

  if (month < 3) {
    month += 12;
    year--;
  }

  return (day + (2 * month) + ((6 * (month + 1)) / 10) + year + (year / 4) -
          (year / 100) + (year / 400) + 1) %
         7;
}

int DayOfWeek(const base::Time time, const bool is_local) {
  base::Time::Exploded exploded;

  if (is_local) {
    time.LocalExplode(&exploded);
  } else {
    time.UTCExplode(&exploded);
  }

  return exploded.day_of_week;
}

}  // namespace brave_ads
