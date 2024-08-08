/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_TIME_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_TIME_TEST_UTIL_H_

#include <string>

#include "base/time/time.h"

namespace brave_ads::test {

// The distance between the past, present and future is only a persistent
// illusion. Albert Einstein.

base::Time DistantPast();
std::string DistantPastAsIso8601();

base::Time Now();
std::string NowAsIso8601();

base::Time DistantFuture();
std::string DistantFutureAsIso8601();

// Converts a string representation of time to a `base::Time` object.
base::Time TimeFromString(const std::string& time_string);

// Converts a string representation of UTC time to a `base::Time` object.
base::Time TimeFromUTCString(const std::string& time_string);

// Converts a string representation of time duration since now to a
// `base::TimeDelta` object.
base::TimeDelta TimeDeltaFromString(const std::string& time_string);

// Converts a string representation of time duration since UTC now to a
// `base::TimeDelta` object.
base::TimeDelta TimeDeltaFromUTCString(const std::string& time_string);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_TIME_TEST_UTIL_H_
