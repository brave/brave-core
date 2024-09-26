/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/common/profiler/thread_profiler_configuration.h"

#define IsProfilerEnabledForCurrentProcessAndThread \
  IsProfilerEnabledForCurrentProcessAndThread_ChromiumImpl

#include "src/chrome/common/profiler/thread_profiler_configuration.cc"

#undef IsProfilerEnabledForCurrentProcessAndThread

bool ThreadProfilerConfiguration::IsProfilerEnabledForCurrentProcessAndThread(
    sampling_profiler::ProfilerThreadType thread) const {
  return false;
}
