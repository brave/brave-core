/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_TIME_CONVERTER_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_TIME_CONVERTER_UTIL_H_

#include <string>

namespace base {
class Time;
class TimeDelta;
}  // namespace base

namespace brave_ads {

// Converts a string representation of time to a `base::Time` object.
base::Time TimeFromString(const std::string& time_string);

// Converts a string representation of UTC time to a `base::Time` object.
base::Time TimeFromUTCString(const std::string& time_string);

// Converts a string representation of time duration since now to a
// `base::TimeDelta` object.
base::TimeDelta TimeDeltaFromString(const std::string& time_string);

// Converts a string representation of UTC time duration since now to a
// `base::TimeDelta` object.
base::TimeDelta TimeDeltaFromUTCString(const std::string& time_string);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_UNITTEST_UNITTEST_TIME_CONVERTER_UTIL_H_
