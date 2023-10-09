/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_util.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNonRewardConfirmationUtilTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    MockConfirmationUserData();

    AdvanceClockTo(
        TimeFromString("Mon, 8 Jul 1996 09:25:00", /*is_local=*/false));
  }
};

TEST_F(BraveAdsNonRewardConfirmationUtilTest, BuildNonRewardConfirmation) {
  // Arrange
  test::DisableBraveRewards();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/false);

  // Act & Assert
  base::MockCallback<BuildConfirmationUserDataCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([&transaction](const UserDataInfo& user_data) {
        ConfirmationInfo expected_confirmation;
        expected_confirmation.transaction_id = kTransactionId;
        expected_confirmation.creative_instance_id = kCreativeInstanceId;
        expected_confirmation.type = ConfirmationType::kViewed;
        expected_confirmation.ad_type = AdType::kNotificationAd;
        expected_confirmation.created_at = Now();

        EXPECT_EQ(expected_confirmation,
                  BuildNonRewardConfirmation(transaction, user_data));
      });

  BuildConfirmationUserData(transaction, callback.Get());
}

TEST_F(BraveAdsNonRewardConfirmationUtilTest,
       DISABLED_DoNotBuildNonRewardConfirmationWithInvalidTransaction) {
  // Arrange
  test::DisableBraveRewards();

  const TransactionInfo transaction;

  // Act & Assert
  EXPECT_DEATH_IF_SUPPORTED(
      BuildNonRewardConfirmation(transaction, /*user_data=*/{}),
      "Check failed: transaction.IsValid*");
}

TEST_F(BraveAdsNonRewardConfirmationUtilTest,
       DISABLED_DoNotBuildNonRewardConfirmationForRewardsUser) {
  // Arrange
  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/false);

  // Act & Assert
  EXPECT_DEATH_IF_SUPPORTED(
      BuildNonRewardConfirmation(transaction, /*user_data=*/{}),
      "Check failed: !UserHasJoinedBraveRewards*");
}

}  // namespace brave_ads
