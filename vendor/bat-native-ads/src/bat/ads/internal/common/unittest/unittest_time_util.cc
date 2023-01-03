/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/common/unittest/unittest_time_util.h"

#include "base/check.h"
#include "base/time/time.h"
#include "base/time/time_to_iso8601.h"

namespace ads {

base::Time TimeFromString(const std::string& time_string, const bool is_local) {
  base::Time time;

  if (is_local) {
    CHECK(base::Time::FromString(time_string.c_str(), &time));
  } else {
    CHECK(base::Time::FromUTCString(time_string.c_str(), &time));
  }

  return time;
}

base::Time DistantPast() {
  return {};
}

std::string DistantPastAsISO8601() {
  return base::TimeToISO8601(DistantPast());
}

base::Time Now() {
  return base::Time::Now();
}

std::string NowAsISO8601() {
  return base::TimeToISO8601(Now());
}

base::Time DistantFuture() {
  // Tuesday, 19 January 2038 03:14:07.
  return base::Time::FromDoubleT(2147483647);
}

std::string DistantFutureAsISO8601() {
  return base::TimeToISO8601(DistantFuture());
}

}  // namespace ads
