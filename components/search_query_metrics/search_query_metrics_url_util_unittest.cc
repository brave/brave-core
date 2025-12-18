/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_query_metrics/search_query_metrics_url_util.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=SearchQueryMetrics*

namespace metrics {

TEST(SearchQueryMetricsUrlUtilTest, Url) {
  EXPECT_EQ(
      GURL("https://anonymous.metrics.bravesoftware.com/v1/metrics/search"),
      GetUrl(/*use_staging=*/true));
  EXPECT_EQ(GURL("https://anonymous.metrics.brave.com/v1/metrics/search"),
            GetUrl(/*use_staging=*/false));
}

}  // namespace metrics
