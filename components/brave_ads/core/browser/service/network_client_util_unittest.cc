/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/network_client_util.h"

#include "net/traffic_annotation/network_traffic_annotation.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsNetworkClientUtilTest, ObliviousHttpKeyConfigUrl) {
  EXPECT_EQ(GURL("https://static.ads.bravesoftware.com/v1/ohttp/hpkekeyconfig"),
            ObliviousHttpKeyConfigUrl(/*use_staging=*/true));
  EXPECT_EQ(GURL("https://static.ads.brave.com/v1/ohttp/hpkekeyconfig"),
            ObliviousHttpKeyConfigUrl(/*use_staging=*/false));
}

TEST(BraveAdsNetworkClientUtilTest, ObliviousHttpRelayUrl) {
  EXPECT_EQ(GURL("https://ohttp.ads.bravesoftware.com/v1/ohttp/gateway"),
            ObliviousHttpRelayUrl(/*use_staging=*/true));
  EXPECT_EQ(GURL("https://ohttp.ads.brave.com/v1/ohttp/gateway"),
            ObliviousHttpRelayUrl(/*use_staging=*/false));
}

TEST(BraveAdsNetworkClientUtilTest, GetNetworkTrafficAnnotationTag) {
  EXPECT_EQ(18366277, GetNetworkTrafficAnnotationTag().unique_id_hash_code);
}

}  // namespace brave_ads
