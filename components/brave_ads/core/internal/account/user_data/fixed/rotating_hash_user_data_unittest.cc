/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/rotating_hash_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRotatingHashUserDataTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    AdvanceClockTo(TimeFromString("2 June 2022 11:00", /*is_local*/ false));
  }
};

TEST_F(BraveAdsRotatingHashUserDataTest,
       BuildRotatingHashUserDataForRewardsUser) {
  // Arrange
  auto& sys_info = GlobalState::GetInstance()->SysInfo();
  sys_info.device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  const TransactionInfo transaction = BuildUnreconciledTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ false);

  // Act

  // Assert
  EXPECT_EQ(
      base::test::ParseJsonDict(
          R"({"rotating_hash":"j9D7eKSoPLYNfxkG2Mx+SbgKJ9hcKg1QwDB8B5qxlpk="})"),
      BuildRotatingHashUserData(transaction));
}

TEST_F(BraveAdsRotatingHashUserDataTest,
       DoNotBuildRotatingHashUserDataForNonRewardsUser) {
  // Arrange
  DisableBraveRewards();

  auto& sys_info = GlobalState::GetInstance()->SysInfo();
  sys_info.device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  const TransactionInfo transaction = BuildUnreconciledTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ false);

  // Act

  // Assert
  EXPECT_TRUE(BuildRotatingHashUserData(transaction).empty());
}

TEST_F(BraveAdsRotatingHashUserDataTest,
       DoNotBuildRotatingHashUserDataIfMissingDeviceId) {
  // Arrange
  const TransactionInfo transaction = BuildUnreconciledTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ false);

  // Act

  // Assert
  EXPECT_EQ(base::Value::Dict(), BuildRotatingHashUserData(transaction));
}

}  // namespace brave_ads
