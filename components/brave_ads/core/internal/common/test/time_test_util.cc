/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"

#include <ctime>

#include "base/check.h"
#include "base/i18n/time_formatting.h"
#include "base/time/time.h"

namespace brave_ads::test {

namespace {

// Returns true if the given time is in daylight saving time.
bool IsDaylightSavingTime(base::Time time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);

  std::tm tm_time = {};
  tm_time.tm_year = exploded.year - 1900;
  tm_time.tm_mon = exploded.month - 1;
  tm_time.tm_mday = exploded.day_of_month;
  tm_time.tm_hour = exploded.hour;
  tm_time.tm_min = exploded.minute;
  tm_time.tm_sec = exploded.second;
  tm_time.tm_isdst = -1;  // Let mktime determine if DST is in effect.

  const std::time_t local_time = std::mktime(&tm_time);
  CHECK_NE(-1, local_time) << "Invalid time";

  return tm_time.tm_isdst > 0;
}

}  // namespace

base::Time DistantPast() {
  // Just after the myth of the beginning of time.
  return base::Time() + base::Milliseconds(1);
}

std::string DistantPastAsIso8601() {
  return base::TimeFormatAsIso8601(DistantPast());
}

base::Time Now() {
  // The time for action is now. It's never too late to do something.
  return base::Time::Now();
}

std::string NowAsIso8601() {
  return base::TimeFormatAsIso8601(Now());
}

base::Time DistantFuture() {
  // Chrome timestamps are 64-bit and will not overflow at 03:14:08 UTC on 19
  // January 2038. However, I only like to think about so far into the future
  // because it comes soon enough.
  return base::Time::FromSecondsSinceUnixEpoch(
      /*Tuesday, 19 January 2038 03:14:07=*/2147483647);
}

std::string DistantFutureAsIso8601() {
  return base::TimeFormatAsIso8601(DistantFuture());
}

base::Time TimeFromString(const std::string& time_string,
                          bool should_adjust_for_dst) {
  base::Time time;
  CHECK(base::Time::FromString(time_string.c_str(), &time));

  if (should_adjust_for_dst && IsDaylightSavingTime(time)) {
    time += base::Hours(1);
  }

  return time;
}

base::Time TimeFromUTCString(const std::string& time_string) {
  base::Time time;
  CHECK(base::Time::FromUTCString(time_string.c_str(), &time));
  return time;
}

base::TimeDelta TimeDeltaFromString(const std::string& time_string,
                                    bool should_adjust_for_dst) {
  return TimeFromString(time_string, should_adjust_for_dst) - base::Time::Now();
}

base::TimeDelta TimeDeltaFromUTCString(const std::string& time_string) {
  return TimeFromUTCString(time_string) - base::Time::Now();
}

}  // namespace brave_ads::test
