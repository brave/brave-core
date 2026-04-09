/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERP_METRICS_TIMEZONE_TEST_UTIL_H_
#define BRAVE_COMPONENTS_SERP_METRICS_TIMEZONE_TEST_UTIL_H_

#include <string>
#include <string_view>
#include <tuple>

#include "base/strings/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace serp_metrics::test {

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

using TimezoneAndReportInUTCParam =
    std::tuple</*timezone=*/std::string_view, /*report_in_utc=*/bool>;

// Sanitizes an IANA time zone identifier into a valid GTest parameter name.
// GTest requires names matching [a-zA-Z0-9_], so '/' is removed; '_' is also
// removed for cleaner output (e.g. "America/New_York" to "AmericaNewYork").
// Also appends `_ReportInUTC` or `_ReportInLocalTime` based on the
// `report_in_utc` parameter.
inline std::string TimezoneAndReportInUtcParamName(
    const ::testing::TestParamInfo<TimezoneAndReportInUTCParam>& test_param) {
  std::string test_param_name;
  base::RemoveChars(std::get<0>(test_param.param), "/_", &test_param_name);
  test_param_name +=
      std::get<1>(test_param.param) ? "_ReportInUTC" : "_ReportInLocalTime";
  return test_param_name;
}

}  // namespace serp_metrics::test

#endif  // BRAVE_COMPONENTS_SERP_METRICS_TIMEZONE_TEST_UTIL_H_
