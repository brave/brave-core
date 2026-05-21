/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/time/time_util.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/calendar/calendar_util.h"

namespace brave_ads {

int MinutesElapsedSinceLocalMidnight(base::Time time) {
  // Explode into local time so the hour and minute fields reflect the wall
  // clock in the device's timezone. Seconds and milliseconds are intentionally
  // excluded; elapsed minutes are truncated, not rounded.
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  const base::TimeDelta time_delta =
      base::Hours(exploded.hour) + base::Minutes(exploded.minute);
  return time_delta.InMinutes();
}

base::Time LocalTimeAtBeginningOfThisMonth() {
  // Subtract enough days to rewind to day 1, then snap to local midnight.
  // Arithmetic is done on the raw wall-clock time so that `LocalMidnight`
  // resolves any daylight saving time offset on the target date.
  const base::Time now = base::Time::Now();
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);
  const int days_elapsed_this_month = exploded.day_of_month - 1;
  return (now - base::Days(days_elapsed_this_month)).LocalMidnight();
}

base::Time LocalTimeAtEndOfThisMonth() {
  // Advance to the first moment of next month, snap to local midnight, then
  // step back 1 millisecond to land at 23:59:59.999 on the last day of this
  // month. Crossing into next month before snapping ensures `LocalMidnight`
  // handles any daylight saving time transition on the last day correctly.
  const base::Time now = base::Time::Now();
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);
  const int days_remaining_in_month =
      DaysInMonth(exploded.year, exploded.month) - exploded.day_of_month;
  const int days_to_start_of_next_month = days_remaining_in_month + 1;
  return (now + base::Days(days_to_start_of_next_month)).LocalMidnight() -
         base::Milliseconds(1);
}

base::Time LocalTimeAtBeginningOfPreviousMonth() {
  // Derive the previous month's year and day count from the already-computed
  // beginning of this month, then subtract that many days and snap to local
  // midnight. Anchoring the subtraction to a known midnight (day 1 of this
  // month) keeps the arithmetic simple and daylight saving time safe.
  const base::Time beginning_of_this_month = LocalTimeAtBeginningOfThisMonth();
  base::Time::Exploded exploded;
  beginning_of_this_month.LocalExplode(&exploded);
  if (--exploded.month == 0) {
    exploded.month = 12;
    --exploded.year;
  }
  const int days_in_previous_month = DaysInMonth(exploded.year, exploded.month);
  return (beginning_of_this_month - base::Days(days_in_previous_month))
      .LocalMidnight();
}

base::Time LocalTimeAtEndOfPreviousMonth() {
  // The millisecond before the beginning of this month is the last moment of
  // the previous month.
  return LocalTimeAtBeginningOfThisMonth() - base::Milliseconds(1);
}

}  // namespace brave_ads
