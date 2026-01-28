/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/serp_metrics_feature.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace metrics {

TEST(SerpMetricsFeatureTest, IsEnabled) {
  EXPECT_TRUE(base::FeatureList::IsEnabled(kSerpMetricsFeature));
}

TEST(SerpMetricsFeatureTest, TimePeriodInDays) {
  EXPECT_EQ(28U, kSerpMetricsTimePeriodInDays.Get());
}

}  // namespace metrics
