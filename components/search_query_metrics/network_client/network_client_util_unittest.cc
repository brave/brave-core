/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_query_metrics/network_client/network_client_util.h"

#include "net/traffic_annotation/network_traffic_annotation.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=SearchQueryMetrics*

namespace metrics {

TEST(SearchQueryMetricsNetworkClientUtilTest, ObliviousHttpKeyConfigUrl) {
  EXPECT_EQ(
      GURL("https://static.metrics.bravesoftware.com/v1/ohttp/hpkekeyconfig"),
      ObliviousHttpKeyConfigUrl(/*use_staging=*/true));
  EXPECT_EQ(GURL("https://static.metrics.brave.com/v1/ohttp/hpkekeyconfig"),
            ObliviousHttpKeyConfigUrl(/*use_staging=*/false));
}

TEST(SearchQueryMetricsNetworkClientUtilTest, ObliviousHttpRelayUrl) {
  EXPECT_EQ(GURL("https://ohttp.metrics.bravesoftware.com/v1/ohttp/gateway"),
            ObliviousHttpRelayUrl(/*use_staging=*/true));
  EXPECT_EQ(GURL("https://ohttp.metrics.brave.com/v1/ohttp/gateway"),
            ObliviousHttpRelayUrl(/*use_staging=*/false));
}

TEST(SearchQueryMetricsNetworkClientUtilTest, GetNetworkTrafficAnnotationTag) {
  EXPECT_EQ(81797308, GetNetworkTrafficAnnotationTag().unique_id_hash_code);
}

}  // namespace metrics
