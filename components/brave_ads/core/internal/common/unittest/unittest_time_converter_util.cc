/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_converter_util.h"

#include "base/check.h"
#include "base/time/time.h"

namespace brave_ads {

base::Time TimeFromString(const std::string& time_string) {
  base::Time time;
  CHECK(base::Time::FromString(time_string.c_str(), &time));
  return time;
}

base::Time TimeFromUTCString(const std::string& time_string) {
  base::Time time;
  CHECK(base::Time::FromUTCString(time_string.c_str(), &time));
  return time;
}

base::TimeDelta TimeDeltaFromString(const std::string& time_string) {
  return TimeFromString(time_string) - base::Time::Now();
}

base::TimeDelta TimeDeltaFromUTCString(const std::string& time_string) {
  return TimeFromUTCString(time_string) - base::Time::Now();
}

}  // namespace brave_ads
