/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_query_metrics/search_query_metrics_environment_util.h"

#include "base/check.h"
#include "base/command_line.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=SearchQueryMetrics*

namespace metrics {

TEST(SearchQueryMetricsEnvironmentUtilTest,
     ShouldUseStagingEnvironmentIfStaging) {
  CHECK(base::CommandLine::InitializedForCurrentProcess());
  base::CommandLine* const command_line =
      base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitchASCII("search-query-metrics", "staging");
  EXPECT_TRUE(ShouldUseStagingEnvironment());
}

TEST(SearchQueryMetricsEnvironmentUtilTest,
     ShouldUseStagingEnvironmentIfStagingIsTrue) {
  CHECK(base::CommandLine::InitializedForCurrentProcess());
  base::CommandLine* const command_line =
      base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitchASCII("search-query-metrics", "staging=true");
  EXPECT_TRUE(ShouldUseStagingEnvironment());
}

TEST(SearchQueryMetricsEnvironmentUtilTest,
     ShouldUseStagingEnvironmentIfStagingIs1) {
  CHECK(base::CommandLine::InitializedForCurrentProcess());
  base::CommandLine* const command_line =
      base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitchASCII("search-query-metrics", "staging=1");
  EXPECT_TRUE(ShouldUseStagingEnvironment());
}

TEST(SearchQueryMetricsEnvironmentUtilTest, ShouldUseProductionEnvironment) {
  EXPECT_FALSE(ShouldUseStagingEnvironment());
}

}  // namespace metrics
