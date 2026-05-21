/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_TIMEZONE_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_TIMEZONE_TEST_UTIL_H_

#include <string>
#include <string_view>

#include "base/strings/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_ads::test {

// Representative IANA time zone identifiers spanning the full UTC offset range
// (UTC−11 to UTC+13), DST/no-DST, whole-hour, and fractional-hour offsets.
inline const auto kTimezones = ::testing::Values(
    "Pacific/Pago_Pago",  // UTC−11, no DST.
    "America/New_York",   // UTC−5/−4, DST, Northern Hemisphere.
    "America/St_Johns",   // UTC−3:30/−2:30, DST, fractional offset.
    "UTC",                // UTC±0, no DST.
    "Europe/Paris",       // UTC+1/+2, DST, Northern Hemisphere.
    "Asia/Kolkata",       // UTC+5:30, no DST, fractional offset.
    "Asia/Tokyo",         // UTC+9, no DST, whole-hour offset.
    "Pacific/Auckland"    // UTC+12/+13, DST, Southern Hemisphere.
);

// Sanitizes an IANA time zone identifier into a valid GTest parameter name.
// GTest requires names matching [a-zA-Z0-9_], so '/' is removed; '_' is also
// removed for cleaner output (e.g. "America/New_York" to "AmericaNewYork").
inline std::string TimezoneTestParamName(
    const ::testing::TestParamInfo<std::string_view>& test_param) {
  std::string test_param_name;
  base::RemoveChars(test_param.param, "/_", &test_param_name);
  return test_param_name;
}

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_COMMON_TEST_TIMEZONE_TEST_UTIL_H_
