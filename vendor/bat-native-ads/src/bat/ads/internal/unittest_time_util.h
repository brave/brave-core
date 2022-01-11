/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_TIME_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_TIME_UTIL_H_

#include <string>

namespace base {
class Time;
}  // namespace base

namespace ads {

base::Time TimeFromString(const std::string& time_string, const bool is_local);
double TimestampFromString(const std::string& time_string, const bool is_local);

base::Time TimestampToTime(const double timestamp);

base::Time MinTime();
base::Time MaxTime();

base::Time DistantPast();
std::string DistantPastAsISO8601();

double NowAsTimestamp();
base::Time Now();
std::string NowAsISO8601();

base::Time DistantFuture();
std::string DistantFutureAsISO8601();

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_UNITTEST_TIME_UTIL_H_
