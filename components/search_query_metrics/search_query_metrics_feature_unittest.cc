/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_query_metrics/search_query_metrics_feature.h"

#include "base/feature_list.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=SearchQueryMetrics*

namespace metrics {

TEST(SearchQueryMetricsFeatureTest, IsEnabled) {
  EXPECT_TRUE(base::FeatureList::IsEnabled(kSearchQueryMetricsFeature));
}

TEST(SearchQueryMetricsFeatureTest, ShouldNotReportForNonRegularProfile) {
  EXPECT_FALSE(kShouldReportForNonRegularProfile.Get());
}

TEST(SearchQueryMetricsFeatureTest, ShouldRetryFailedReports) {
  EXPECT_TRUE(kShouldRetryFailedReports.Get());
}

TEST(SearchQueryMetricsFeatureTest, InitialBackoffDelay) {
  EXPECT_EQ(base::Minutes(15), kInitialBackoffDelay.Get());
}

TEST(SearchQueryMetricsFeatureTest, MaxBackoffDelay) {
  EXPECT_EQ(base::Days(1), kMaxBackoffDelay.Get());
}

TEST(SearchQueryMetricsFeatureTest, MaxRetryCount) {
  EXPECT_EQ(5, kMaxRetryCount.Get());
}

}  // namespace metrics
