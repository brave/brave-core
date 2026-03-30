/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/creative_ads_database_table.h"

#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsCreativeAdsDatabaseTableTest : public test::TestBase {};

TEST_F(BraveAdsCreativeAdsDatabaseTableTest,
       DoNotGetCreativeAdForMissingCreativeInstanceId) {
  // Act & Assert
  base::test::TestFuture<bool, std::string, CreativeAdInfo> test_future;
  const CreativeAds database_table;
  database_table.GetForCreativeInstanceId(
      test::kMissingCreativeInstanceId,
      test_future
          .GetCallback<bool, const std::string&, const CreativeAdInfo&>());
  const auto [success, creative_instance_id, creative_ad] = test_future.Take();
  EXPECT_FALSE(success);
}

}  // namespace brave_ads::database::table
