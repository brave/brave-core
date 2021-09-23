/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/unittest_time_util.h"

#include "base/time/time_to_iso8601.h"

namespace ads {

int64_t TimestampFromDateString(const std::string& date) {
  return TimeFromDateString(date).ToDoubleT();
}

base::Time TimeFromDateString(const std::string& date) {
  base::Time time;
  if (!base::Time::FromUTCString(date.c_str(), &time)) {
    return time;
  }

  return time.UTCMidnight() + base::TimeDelta::FromDays(1) -
         base::TimeDelta::FromMilliseconds(1);
}

int64_t DistantPastAsTimestamp() {
  return 0;  // Thursday, 1 January 1970 00:00:00
}

base::Time DistantPast() {
  return base::Time::FromDoubleT(DistantPastAsTimestamp());
}

std::string DistantPastAsISO8601() {
  return base::TimeToISO8601(DistantPast());
}

int64_t NowAsTimestamp() {
  return static_cast<int64_t>(Now().ToDoubleT());
}

base::Time Now() {
  return base::Time::Now();
}

std::string NowAsISO8601() {
  return base::TimeToISO8601(Now());
}

int64_t DistantFutureAsTimestamp() {
  return 4102444799;  // Thursday, 31 December 2099 23:59:59
}

base::Time DistantFuture() {
  return base::Time::FromDoubleT(DistantFutureAsTimestamp());
}

std::string DistantFutureAsISO8601() {
  return base::TimeToISO8601(DistantFuture());
}

}  // namespace ads
