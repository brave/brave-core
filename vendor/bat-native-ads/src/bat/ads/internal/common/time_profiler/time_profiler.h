/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_TIME_PROFILER_TIME_PROFILER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_TIME_PROFILER_TIME_PROFILER_H_

#include "bat/ads/internal/common/time_profiler/time_profiler_manager.h"

namespace ads {

// Example usage:
//
//   TIME_PROFILER_BEGIN("someCategory");
//   TIME_PROFILER_MEASURE_WITH_MESSAGE("someCategory", "SomeMessage");
//   TIME_PROFILER_MEASURE("someCategory");
//   TIME_PROFILER_END("someCategory");
//
// Measures and logs the elapsed time ticks between each optional
// |TIME_PROFILER_MEASURE*| call and the total elapsed time ticks after calling
// |TIME_PROFILER_END| in milliseconds.
//
// You must call |TIME_PROFILER_BEGIN| before calling |TIME_PROFILER_MEASURE*|,
// |TIME_PROFILER_RESET| or |TIME_PROFILER_END|. Logs are logged at verbose
// level 6 or higher.

#define TIME_PROFILER_BEGIN(category_group)                 \
  TimeProfilerManager::GetInstance()->Begin(category_group, \
                                            __PRETTY_FUNCTION__);
#define TIME_PROFILER_MEASURE_WITH_MESSAGE(category_group, message) \
  TimeProfilerManager::GetInstance()->Measure(                      \
      category_group, __PRETTY_FUNCTION__, __LINE__, message);
#define TIME_PROFILER_MEASURE(category_group)                 \
  TimeProfilerManager::GetInstance()->Measure(category_group, \
                                              __PRETTY_FUNCTION__, __LINE__);
#define TIME_PROFILER_END(category_group) \
  TimeProfilerManager::GetInstance()->End(category_group, __PRETTY_FUNCTION__);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BASE_TIME_PROFILER_TIME_PROFILER_H_
