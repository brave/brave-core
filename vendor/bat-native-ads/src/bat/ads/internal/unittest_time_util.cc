/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/unittest_time_util.h"

#include <limits>

#include "base/check.h"
#include "base/time/time.h"
#include "base/time/time_to_iso8601.h"

namespace ads {

base::Time TimeFromUTCString(const std::string& time_string) {
  base::Time time;
  const bool success = base::Time::FromUTCString(time_string.c_str(), &time);
  DCHECK(success);

  return time;
}

double UTCTimeStringToTimestamp(const std::string& time_string) {
  return TimeToTimestamp(TimeFromUTCString(time_string));
}

base::Time TimestampToTime(const double timestamp) {
  return base::Time::FromDoubleT(timestamp);
}

double TimeToTimestamp(const base::Time& time) {
  return time.ToDoubleT();
}

base::Time MinTime() {
  return TimestampToTime(std::numeric_limits<double>::min());
}

base::Time MaxTime() {
  return TimestampToTime(std::numeric_limits<double>::max());
}

base::Time DistantPast() {
  return base::Time();
}

std::string DistantPastAsISO8601() {
  return base::TimeToISO8601(DistantPast());
}

double NowAsTimestamp() {
  return Now().ToDoubleT();
}

base::Time Now() {
  return base::Time::Now();
}

std::string NowAsISO8601() {
  return base::TimeToISO8601(Now());
}

base::Time DistantFuture() {
  // Thursday, 31 December 2099 23:59:59
  return base::Time::FromDoubleT(4102444799);
}

std::string DistantFutureAsISO8601() {
  return base::TimeToISO8601(DistantFuture());
}

}  // namespace ads
