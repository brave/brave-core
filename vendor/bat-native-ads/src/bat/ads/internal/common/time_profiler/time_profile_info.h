/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_TIME_PROFILER_TIME_PROFILE_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_TIME_PROFILER_TIME_PROFILE_INFO_H_

#include <string>

#include "base/time/time.h"

namespace ads {

struct TimeProfileInfo {
  int indent_level = 0;
  std::string name;
  base::TimeTicks start_time_ticks;
  base::TimeTicks last_time_ticks;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_TIME_PROFILER_TIME_PROFILE_INFO_H_
