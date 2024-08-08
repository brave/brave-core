/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_test_constants.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/account/user_data/user_data_info.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRewardConfirmationUtilTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    test::MockConfirmationUserData();

    AdvanceClockTo(test::TimeFromUTCString("Mon, 8 Jul 1996 09:25"));
  }
};

TEST_F(BraveAdsRewardConfirmationUtilTest, BuildRewardCredential) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  // Act
  const std::optional<std::string> reward_credential =
      BuildRewardCredential(*confirmation);
  ASSERT_TRUE(reward_credential);

  // Assert
  EXPECT_EQ(
      R"(eyJzaWduYXR1cmUiOiJNem1obU8zak5rbDVNOHBGNG96ejJaVlMycVdKblJwaWJSR1B0UmJNMWY4NHhPZWhmQmVBdkxsbkpJSlVObStvNmtQMkxXYTF0TkFIT2VjRGF6dUlGUT09IiwidCI6IlBMb3d6MldGMmVHRDV6Zndaams5cDc2SFhCTERLTXEvM0VBWkhlRy9mRTJYR1E0OGp5dGUrVmU1MFpsYXNPdVlMNW13QThDVTJhRk1sSnJ0M0REZ0N3PT0ifQ==)",
      reward_credential);
}

TEST_F(BraveAdsRewardConfirmationUtilTest, BuildRewardConfirmation) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/false);

  // Act
  const std::optional<ConfirmationInfo> confirmation =
      BuildRewardConfirmation(transaction, /*user_data=*/{});
  ASSERT_TRUE(confirmation);

  // Assert
  const RewardInfo expected_reward = test::BuildReward(*confirmation);

  UserDataInfo expected_user_data;
  expected_user_data.dynamic = base::test::ParseJsonDict(
      R"(
          {
            "diagnosticId": "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2",
            "systemTimestamp": "1996-07-08T09:00:00.000Z"
          })");
  expected_user_data.fixed = base::test::ParseJsonDict(
      R"(
          {
            "buildChannel": "release",
            "catalog": [
              {
                "id": "29e5c8bc0ba319069980bb390d8e8f9b58c05a20"
              }
            ],
            "createdAtTimestamp": "1996-07-08T09:00:00.000Z",
            "platform": "windows",
            "rotatingHash": "jBdiJH7Hu3wj31WWNLjKV5nVxFxWSDWkYh5zXCS3rXY=",
            "segment": "untargeted",
            "studies": [],
            "versionNumber": "1.2.3.4"
          })");

  EXPECT_THAT(
      *confirmation,
      ::testing::FieldsAre(
          test::kTransactionId, test::kCreativeInstanceId,
          ConfirmationType::kViewedImpression, AdType::kNotificationAd,
          /*created_at*/ test::Now(), expected_reward, expected_user_data));
}

TEST_F(BraveAdsRewardConfirmationUtilTest,
       DoNotBuildRewardConfirmationIfNoConfirmationTokens) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/false);

  // Act
  const std::optional<ConfirmationInfo> confirmation =
      BuildRewardConfirmation(transaction, /*user_data=*/{});

  // Assert
  EXPECT_FALSE(confirmation);
}

TEST_F(BraveAdsRewardConfirmationUtilTest,
       DISABLED_DoNotBuildRewardConfirmationWithInvalidTransaction) {
  // Act & Assert
  EXPECT_DEATH_IF_SUPPORTED(BuildRewardConfirmation(/*transaction=*/{},
                                                    /*user_data=*/{}),
                            "Check failed: transaction.IsValid*");
}

TEST_F(BraveAdsRewardConfirmationUtilTest,
       DISABLED_DoNotBuildRewardConfirmationForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/false);

  // Act & Assert
  EXPECT_DEATH_IF_SUPPORTED(BuildRewardConfirmation(transaction,
                                                    /*user_data=*/{}),
                            "Check failed: UserHasJoinedBraveRewards*");
}

}  // namespace brave_ads
