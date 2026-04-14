/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_table.h"

#include <optional>

#include "base/test/mock_callback.h"
#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposit_info.h"
#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsDepositsDatabaseTableTest : public test::TestBase {};

TEST_F(BraveAdsDepositsDatabaseTableTest,
       DoNotGetDepositForMissingCreativeInstanceId) {
  // Act & Assert
  base::test::TestFuture<bool, std::optional<DepositInfo>> test_future;
  const Deposits database_table;
  database_table.GetForCreativeInstanceId(
      test::kMissingCreativeInstanceId,
      test_future.GetCallback<bool, std::optional<DepositInfo>>());
  const auto [success, deposit] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_FALSE(deposit);
}

TEST_F(BraveAdsDepositsDatabaseTableTest, SaveDeposit) {
  // Arrange
  DepositInfo deposit;
  deposit.creative_instance_id = test::kCreativeInstanceId;
  deposit.value = 1.0;
  deposit.expire_at = test::DistantFuture();

  // Act & Assert
  base::test::TestFuture<bool> test_future;
  Deposits database_table;
  database_table.Save(deposit, test_future.GetCallback());
  EXPECT_TRUE(test_future.Take());
}

TEST_F(BraveAdsDepositsDatabaseTableTest, GetDepositForCreativeInstanceId) {
  // Arrange
  DepositInfo deposit;
  deposit.creative_instance_id = test::kCreativeInstanceId;
  deposit.value = 1.0;
  deposit.expire_at = test::DistantFuture();

  base::MockCallback<ResultCallback> save_callback;
  EXPECT_CALL(save_callback, Run(/*success=*/true));
  Deposits database_table;
  database_table.Save(deposit, save_callback.Get());

  // Act & Assert
  base::test::TestFuture<bool, std::optional<DepositInfo>> test_future;
  database_table.GetForCreativeInstanceId(
      test::kCreativeInstanceId,
      test_future.GetCallback<bool, std::optional<DepositInfo>>());
  const auto [success, result] = test_future.Take();
  EXPECT_TRUE(success);
  ASSERT_TRUE(result);
  EXPECT_EQ(test::kCreativeInstanceId, result->creative_instance_id);
  EXPECT_DOUBLE_EQ(1.0, result->value);
}

}  // namespace brave_ads::database::table
