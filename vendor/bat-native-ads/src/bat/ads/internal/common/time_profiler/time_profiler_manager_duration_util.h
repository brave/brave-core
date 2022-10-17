/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_TIME_PROFILER_TIME_PROFILER_MANAGER_DURATION_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_TIME_PROFILER_TIME_PROFILER_MANAGER_DURATION_UTIL_H_

#include <string>

namespace base {
class TimeTicks;
}  // namespace base

namespace ads {

struct TimeProfileInfo;

std::string GetDurationSinceLastTimeTicks(const base::TimeTicks& time_ticks);

std::string BuildDurationSinceLastTimeTicksLogMessage(
    const std::string& category_group,
    const int line,
    const std::string& message,
    const TimeProfileInfo& time_profile);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_TIME_PROFILER_TIME_PROFILER_MANAGER_DURATION_UTIL_H_
