/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/time_util.h"

#include "base/check.h"
#include "base/time/time.h"
#include "bat/ads/internal/calendar_util.h"

namespace ads {

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
  DCHECK(success);

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
  DCHECK(success);

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
  DCHECK(success);

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
  DCHECK(success);

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

}  // namespace ads
