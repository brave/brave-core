/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_TIME_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_TIME_UTIL_H_

#include <string>

namespace base {
class Time;
}  // namespace base

namespace brave_ads {

base::Time TimeFromString(const std::string& time_string, bool is_local);

// The distance between the past, present and future is only a persistent
// illusion. Albert Einstein.

base::Time DistantPast();
std::string DistantPastAsIso8601();

base::Time Now();
std::string NowAsIso8601();

base::Time DistantFuture();
std::string DistantFutureAsIso8601();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_TIME_UTIL_H_
