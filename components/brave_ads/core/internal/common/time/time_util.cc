/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/time/time_util.h"

#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/calendar/calendar_util.h"

namespace brave_ads {

int64_t ToChromeTimestampFromTime(const base::Time time) {
  return time.ToDeltaSinceWindowsEpoch().InMicroseconds();
}

base::Time ToTimeFromChromeTimestamp(const int64_t timestamp) {
  return base::Time::FromDeltaSinceWindowsEpoch(base::Microseconds(timestamp));
}

int GetLocalTimeInMinutes(const base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);

  const base::TimeDelta time_delta =
      base::Hours(exploded.hour) + base::Minutes(exploded.minute);
  return time_delta.InMinutes();
}

base::Time AdjustLocalTimeToBeginningOfPreviousMonth(const base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);

  --exploded.month;
  if (exploded.month < 1) {
    exploded.month = 12;
    --exploded.year;
  }

  exploded.day_of_month = 1;

  exploded.day_of_week =
      DayOfWeek(exploded.year, exploded.month, exploded.day_of_month);

  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;

  base::Time adjusted_time;
  const bool success = base::Time::FromLocalExploded(exploded, &adjusted_time);
  DCHECK(success);

  return adjusted_time;
}

base::Time AdjustLocalTimeToEndOfPreviousMonth(const base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);

  --exploded.month;
  if (exploded.month < 1) {
    exploded.month = 12;
    --exploded.year;
  }

  exploded.day_of_month = DaysPerMonth(exploded.year, exploded.month);

  exploded.day_of_week =
      DayOfWeek(exploded.year, exploded.month, exploded.day_of_month);

  exploded.hour = 23;
  exploded.minute = 59;
  exploded.second = 59;
  exploded.millisecond = 999;

  base::Time adjusted_time;
  const bool success = base::Time::FromLocalExploded(exploded, &adjusted_time);
  DCHECK(success);

  return adjusted_time;
}

base::Time AdjustLocalTimeToBeginningOfMonth(const base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);

  exploded.day_of_month = 1;

  exploded.day_of_week =
      DayOfWeek(exploded.year, exploded.month, exploded.day_of_month);

  exploded.hour = 0;
  exploded.minute = 0;
  exploded.second = 0;
  exploded.millisecond = 0;

  base::Time adjusted_time;
  const bool success = base::Time::FromLocalExploded(exploded, &adjusted_time);
  DCHECK(success);

  return adjusted_time;
}

base::Time AdjustLocalTimeToEndOfMonth(const base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);

  exploded.day_of_month = DaysPerMonth(exploded.year, exploded.month);

  exploded.day_of_week =
      DayOfWeek(exploded.year, exploded.month, exploded.day_of_month);

  exploded.hour = 23;
  exploded.minute = 59;
  exploded.second = 59;
  exploded.millisecond = 999;

  base::Time adjusted_time;
  const bool success = base::Time::FromLocalExploded(exploded, &adjusted_time);
  DCHECK(success);

  return adjusted_time;
}

base::Time GetTimeInDistantPast() {
  return {};
}

base::Time GetLocalTimeAtBeginningOfLastMonth() {
  const base::Time now = base::Time::Now();
  return AdjustLocalTimeToBeginningOfPreviousMonth(now);
}

base::Time GetLocalTimeAtEndOfLastMonth() {
  const base::Time now = base::Time::Now();
  return AdjustLocalTimeToEndOfPreviousMonth(now);
}

base::Time GetLocalTimeAtBeginningOfThisMonth() {
  const base::Time now = base::Time::Now();
  return AdjustLocalTimeToBeginningOfMonth(now);
}

base::Time GetLocalTimeAtEndOfThisMonth() {
  const base::Time now = base::Time::Now();
  return AdjustLocalTimeToEndOfMonth(now);
}

std::string TimeToPrivacyPreservingIso8601(const base::Time time) {
  base::Time::Exploded exploded;
  time.UTCExplode(&exploded);

  return base::StringPrintf("%04d-%02d-%02dT%02d:00:00.000Z", exploded.year,
                            exploded.month, exploded.day_of_month,
                            exploded.hour);
}

}  // namespace brave_ads
