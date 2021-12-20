/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/time_util.h"

#include "base/check.h"
#include "base/time/time.h"
#include "bat/ads/internal/calendar_util.h"

namespace ads {

namespace {

bool g_from_local_exploded_failed = false;

// TODO(https://github.com/brave/brave-browser/issues/20169): Remove this
// function when base::Time::FromLocalExploded for linux sandbox will be fixed.
base::Time CalculateBeginningOfMonth(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  DCHECK(exploded.HasValidValues());

  base::Time adjusted_time =
      GetLocalMidnight(time - base::Days(exploded.day_of_month - 1), exploded);
  return adjusted_time;
}

// TODO(https://github.com/brave/brave-browser/issues/20169): Remove this
// function when base::Time::FromLocalExploded for linux sandbox will be fixed.
base::Time CalculateEndOfPreviousMonth(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  DCHECK(exploded.HasValidValues());

  base::Time adjusted_time =
      GetLocalMidnight(time - base::Days(exploded.day_of_month - 1), exploded);
  adjusted_time -= base::Milliseconds(1);
  return adjusted_time;
}

// TODO(https://github.com/brave/brave-browser/issues/20169): Remove this
// function when base::Time::FromLocalExploded for linux sandbox will be fixed.
base::Time CalculateBeginningOfPreviousMonth(const base::Time& time) {
  base::Time end_prev_month = CalculateEndOfPreviousMonth(time);
  return CalculateBeginningOfMonth(end_prev_month);
}

// TODO(https://github.com/brave/brave-browser/issues/20169): Remove this
// function when base::Time::FromLocalExploded for linux sandbox will be fixed.
base::Time CalculateEndOfMonth(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  DCHECK(exploded.HasValidValues());

  const base::Time next_month_first_day =
      time + base::Days(GetLastDayOfMonth(exploded.year, exploded.month) -
                        exploded.day_of_month + 1);
  base::Time adjusted_time = GetLocalMidnight(next_month_first_day, exploded);
  adjusted_time -= base::Milliseconds(1);

  return adjusted_time;
}

}  // namespace

// TODO(https://github.com/brave/brave-browser/issues/20169): Remove this
// function when base::Time::FromLocalExploded for linux sandbox will be fixed.
base::Time GetLocalMidnight(const base::Time& time,
                            const base::Time::Exploded& exploded) {
  base::Time midnight =
      time - base::Hours(exploded.hour) - base::Minutes(exploded.minute) -
      base::Seconds(exploded.second) - base::Milliseconds(exploded.millisecond);

  // Check for errors due to daylight saving time change.
  base::Time::Exploded midnight_exploded;
  midnight.LocalExplode(&midnight_exploded);
  DCHECK(midnight_exploded.HasValidValues());

  if (midnight_exploded.hour != 0) {
    if (midnight_exploded.day_of_month == exploded.day_of_month) {
      midnight -= base::Hours(1);
    } else {
      midnight += base::Hours(1);
    }
  }

  return midnight;
}

int GetLocalTimeAsMinutes(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  DCHECK(exploded.HasValidValues());

  return (exploded.hour * base::Time::kMinutesPerHour) + exploded.minute;
}

base::Time AdjustTimeToBeginningOfPreviousMonth(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  DCHECK(exploded.HasValidValues());

  exploded.month--;
  if (exploded.month < 1) {
    exploded.month = 12;
    exploded.year--;
  }

  exploded.day_of_month = 1;

  exploded.day_of_week =
      GetDayOfWeek(exploded.year, exploded.month, exploded.day_of_month);

  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;

  base::Time adjusted_time;
  const bool success = base::Time::FromLocalExploded(exploded, &adjusted_time);
  if (!success || g_from_local_exploded_failed) {
    return CalculateBeginningOfPreviousMonth(time);
  }

  return adjusted_time;
}

base::Time AdjustTimeToEndOfPreviousMonth(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  DCHECK(exploded.HasValidValues());

  exploded.month--;
  if (exploded.month < 1) {
    exploded.month = 12;
    exploded.year--;
  }

  exploded.day_of_month = GetLastDayOfMonth(exploded.year, exploded.month);

  exploded.day_of_week =
      GetDayOfWeek(exploded.year, exploded.month, exploded.day_of_month);

  exploded.hour = 23;
  exploded.minute = 59;
  exploded.second = 59;
  exploded.millisecond = 999;

  base::Time adjusted_time;
  const bool success = base::Time::FromLocalExploded(exploded, &adjusted_time);
  if (!success || g_from_local_exploded_failed) {
    return CalculateEndOfPreviousMonth(time);
  }

  return adjusted_time;
}

base::Time AdjustTimeToBeginningOfMonth(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  DCHECK(exploded.HasValidValues());

  exploded.day_of_month = 1;

  exploded.day_of_week =
      GetDayOfWeek(exploded.year, exploded.month, exploded.day_of_month);

  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;

  base::Time adjusted_time;
  const bool success = base::Time::FromLocalExploded(exploded, &adjusted_time);
  if (!success || g_from_local_exploded_failed) {
    return CalculateBeginningOfMonth(time);
  }

  return adjusted_time;
}

base::Time AdjustTimeToEndOfMonth(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  DCHECK(exploded.HasValidValues());

  exploded.day_of_month = GetLastDayOfMonth(exploded.year, exploded.month);

  exploded.day_of_week =
      GetDayOfWeek(exploded.year, exploded.month, exploded.day_of_month);

  exploded.hour = 23;
  exploded.minute = 59;
  exploded.second = 59;
  exploded.millisecond = 999;

  base::Time adjusted_time;
  const bool success = base::Time::FromLocalExploded(exploded, &adjusted_time);
  if (!success || g_from_local_exploded_failed) {
    return CalculateEndOfMonth(time);
  }

  return adjusted_time;
}

base::Time GetTimeAtBeginningOfLastMonth() {
  const base::Time& now = base::Time::Now();
  return AdjustTimeToBeginningOfPreviousMonth(now);
}

base::Time GetTimeAtEndOfLastMonth() {
  const base::Time& now = base::Time::Now();
  return AdjustTimeToEndOfPreviousMonth(now);
}

base::Time GetTimeAtBeginningOfThisMonth() {
  const base::Time& now = base::Time::Now();
  return AdjustTimeToBeginningOfMonth(now);
}

base::Time GetTimeAtEndOfThisMonth() {
  const base::Time& now = base::Time::Now();
  return AdjustTimeToEndOfMonth(now);
}

void SetFromLocalExplodedFailedForTesting(bool set_failed) {
  g_from_local_exploded_failed = set_failed;
}

}  // namespace ads
