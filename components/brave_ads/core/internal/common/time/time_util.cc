/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/time/time_util.h"

#include "base/check_is_test.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/calendar/calendar_util.h"

namespace brave_ads {

namespace {

bool g_from_local_exploded_failed_for_testing = false;

// TODO(https://github.com/brave/brave-browser/issues/20169): Remove this
// function when base::Time::FromLocalExploded for linux sandbox will be fixed.
base::Time CorrectLocalMidnightForDaylightSaving(const base::Time midnight,
                                                 int expected_day_of_month) {
  // Check for errors due to daylight saving time change.
  base::Time::Exploded midnight_exploded;
  midnight.LocalExplode(&midnight_exploded);

  base::Time corrected_midnight = midnight;
  if (midnight_exploded.hour != 0) {
    if (midnight_exploded.day_of_month == expected_day_of_month) {
      corrected_midnight -= base::Hours(1);
    } else {
      corrected_midnight += base::Hours(1);
    }
  }
  return corrected_midnight;
}

// TODO(https://github.com/brave/brave-browser/issues/20169): Remove this
// function when base::Time::FromLocalExploded for linux sandbox will be fixed.
base::Time CalculateBeginningOfMonth(const base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);

  const base::Time midnight = GetLocalMidnight(time);
  const base::Time shifted_midnight =
      midnight - base::Days(exploded.day_of_month - 1);
  return CorrectLocalMidnightForDaylightSaving(shifted_midnight,
                                               /*expected_day_of_month=*/1);
}

// TODO(https://github.com/brave/brave-browser/issues/20169): Remove this
// function when base::Time::FromLocalExploded for linux sandbox will be fixed.
base::Time CalculateBeginningOfNextMonth(const base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);

  const base::Time midnight = GetLocalMidnight(time);
  const base::Time shifted_midnight =
      midnight + base::Days(DaysPerMonth(exploded.year, exploded.month) -
                            exploded.day_of_month + 1);
  return CorrectLocalMidnightForDaylightSaving(shifted_midnight,
                                               /*expected_day_of_month=*/1);
}

// TODO(https://github.com/brave/brave-browser/issues/20169): Remove this
// function when base::Time::FromLocalExploded for linux sandbox will be fixed.
base::Time CalculateEndOfPreviousMonth(const base::Time time) {
  base::Time adjusted_time = CalculateBeginningOfMonth(time);
  adjusted_time -= base::Milliseconds(1);
  return adjusted_time;
}

// TODO(https://github.com/brave/brave-browser/issues/20169): Remove this
// function when base::Time::FromLocalExploded for linux sandbox will be fixed.
base::Time CalculateBeginningOfPreviousMonth(const base::Time time) {
  const base::Time end_previous_month = CalculateEndOfPreviousMonth(time);
  return CalculateBeginningOfMonth(end_previous_month);
}

// TODO(https://github.com/brave/brave-browser/issues/20169): Remove this
// function when base::Time::FromLocalExploded for linux sandbox will be fixed.
base::Time CalculateEndOfMonth(const base::Time time) {
  base::Time adjusted_time = CalculateBeginningOfNextMonth(time);
  adjusted_time -= base::Milliseconds(1);
  return adjusted_time;
}

}  // namespace

// TODO(https://github.com/brave/brave-browser/issues/20169): Remove this
// function when base::Time::FromLocalExploded for linux sandbox will be fixed.
base::Time GetLocalMidnight(const base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);

  const base::Time midnight =
      time - base::Hours(exploded.hour) - base::Minutes(exploded.minute) -
      base::Seconds(exploded.second) - base::Milliseconds(exploded.millisecond);

  return CorrectLocalMidnightForDaylightSaving(midnight, exploded.day_of_month);
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
  if (!success || g_from_local_exploded_failed_for_testing) {
    return CalculateBeginningOfPreviousMonth(time);
  }

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
  if (!success || g_from_local_exploded_failed_for_testing) {
    return CalculateEndOfPreviousMonth(time);
  }

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
  if (!success || g_from_local_exploded_failed_for_testing) {
    return CalculateBeginningOfMonth(time);
  }

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
  if (!success || g_from_local_exploded_failed_for_testing) {
    return CalculateEndOfMonth(time);
  }

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

void SetFromLocalExplodedFailedForTesting(const bool set_failed) {
  CHECK_IS_TEST();

  g_from_local_exploded_failed_for_testing = set_failed;
}

}  // namespace brave_ads
