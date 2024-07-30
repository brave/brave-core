/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/common/profiler/thread_profiler_configuration.h"

#include "testing/gtest/include/gtest/gtest.h"

TEST(ThreadProfilerConfigurationTest, DisabledForAllThreads) {
  const auto* config = ThreadProfilerConfiguration::Get();
  for (int t = static_cast<int>(base::ProfilerThreadType::kUnknown);
       t <= static_cast<int>(base::ProfilerThreadType::kMax); ++t) {
    // This should be disabled for all threads.
    EXPECT_FALSE(config->IsProfilerEnabledForCurrentProcessAndThread(
        static_cast<base::ProfilerThreadType>(t)))
        << t;
  }
}
