/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/time/time_util.h"

#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/calendar/calendar_util.h"

namespace brave_ads {

int LocalTimeInMinutesSinceMidnight(const base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  const base::TimeDelta time_delta =
      base::Hours(exploded.hour) + base::Minutes(exploded.minute);
  return time_delta.InMinutes();
}

base::Time LocalTimeAtBeginningOfPreviousMonth() {
  const base::Time now = base::Time::Now().LocalMidnight();

  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);
  exploded.month--;
  if (exploded.month == 0) {
    exploded.month = 12;
    exploded.year--;
  }
  const int days_in_previous_month = DaysInMonth(exploded.year, exploded.month);
  const int current_day_of_month = exploded.day_of_month - 1;
  const int days_since_start_of_previous_month =
      current_day_of_month + days_in_previous_month;

  return now - base::Days(days_since_start_of_previous_month);
}

base::Time LocalTimeAtEndOfPreviousMonth() {
  return LocalTimeAtBeginningOfThisMonth() - base::Milliseconds(1);
}

base::Time LocalTimeAtBeginningOfThisMonth() {
  const base::Time now = base::Time::Now().LocalMidnight();

  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);
  const int current_day_of_month = exploded.day_of_month - 1;

  return now - base::Days(current_day_of_month);
}

base::Time LocalTimeAtEndOfThisMonth() {
  const base::Time now = base::Time::Now().LocalMidnight();

  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);
  const int days_in_month = DaysInMonth(exploded.year, exploded.month);
  const int current_day_of_month = exploded.day_of_month - 1;
  const int remaining_days_in_month = days_in_month - current_day_of_month;

  return now + base::Days(remaining_days_in_month) - base::Milliseconds(1);
}

std::string TimeToPrivacyPreservingIso8601(const base::Time time) {
  base::Time::Exploded exploded;
  time.UTCExplode(&exploded);

  return base::StringPrintf("%04d-%02d-%02dT%02d:00:00.000Z", exploded.year,
                            exploded.month, exploded.day_of_month,
                            exploded.hour);
}

}  // namespace brave_ads
