/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_util.h"

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_test_constants.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNonRewardConfirmationUtilTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    test::MockConfirmationUserData();
  }
};

TEST_F(BraveAdsNonRewardConfirmationUtilTest, BuildNonRewardConfirmation) {
  // Arrange
  test::DisableBraveRewards();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/false);

  // Act
  const std::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(transaction, /*user_data=*/{});
  ASSERT_TRUE(confirmation);

  // Assert
  EXPECT_THAT(*confirmation,
              ::testing::FieldsAre(
                  test::kTransactionId, test::kCreativeInstanceId,
                  ConfirmationType::kViewedImpression, AdType::kNotificationAd,
                  /*created_at*/ test::Now(),
                  /*reward*/ std::nullopt, UserDataInfo{}));
}

TEST_F(BraveAdsNonRewardConfirmationUtilTest,
       DISABLED_DoNotBuildNonRewardConfirmationWithInvalidTransaction) {
  // Arrange
  test::DisableBraveRewards();

  // Act
  const std::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(/*transaction=*/{}, /*user_data=*/{});
  ASSERT_TRUE(confirmation);

  // Assert
  EXPECT_DEATH_IF_SUPPORTED(*confirmation,
                            "Check failed: transaction.IsValid*");
}

TEST_F(BraveAdsNonRewardConfirmationUtilTest,
       DISABLED_DoNotBuildNonRewardConfirmationForRewardsUser) {
  // Arrange
  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/false);

  // Act
  const std::optional<ConfirmationInfo> confirmation =
      BuildNonRewardConfirmation(transaction, /*user_data=*/{});
  ASSERT_TRUE(confirmation);

  // Assert
  EXPECT_DEATH_IF_SUPPORTED(*confirmation,
                            "Check failed: !UserHasJoinedBraveRewards*");
}

}  // namespace brave_ads
