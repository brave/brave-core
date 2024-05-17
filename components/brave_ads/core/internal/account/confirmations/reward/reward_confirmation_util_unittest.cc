/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_converter_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRewardConfirmationUtilTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    MockConfirmationUserData();

    AdvanceClockTo(TimeFromUTCString("Mon, 8 Jul 1996 09:25"));
  }

  TokenGeneratorMock token_generator_mock_;
};

TEST_F(BraveAdsRewardConfirmationUtilTest, BuildRewardCredential) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(&token_generator_mock_,
                                    /*should_use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  // Act & Assert
  EXPECT_EQ(
      R"(eyJzaWduYXR1cmUiOiJjZjBLajJpbVB6d0ZOYk9NMEtnSGYxNVkxRmRNY1cxaS90R2hwbVVWbUhqK25KbXphNko1NkkrUmpVd3A3alJOZ3lMR2lQcTUwZm1mZ1Y5WEorTDBOUT09IiwidCI6IlBMb3d6MldGMmVHRDV6Zndaams5cDc2SFhCTERLTXEvM0VBWkhlRy9mRTJYR1E0OGp5dGUrVmU1MFpsYXNPdVlMNW13QThDVTJhRk1sSnJ0M0REZ0N3PT0ifQ==)",
      BuildRewardCredential(*confirmation));
}

TEST_F(BraveAdsRewardConfirmationUtilTest, BuildRewardConfirmation) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*should_use_random_uuids=*/false);

  // Act & Assert
  const std::optional<ConfirmationInfo> confirmation = BuildRewardConfirmation(
      &token_generator_mock_, transaction, /*user_data=*/{});
  ASSERT_TRUE(confirmation);

  ConfirmationInfo expected_confirmation;

  expected_confirmation.transaction_id = kTransactionId;
  expected_confirmation.creative_instance_id = kCreativeInstanceId;
  expected_confirmation.type = ConfirmationType::kViewedImpression;
  expected_confirmation.ad_type = AdType::kNotificationAd;
  expected_confirmation.created_at = Now();

  expected_confirmation.reward = test::BuildReward(*confirmation);

  expected_confirmation.user_data.dynamic = base::test::ParseJsonDict(
      R"(
            {
              "diagnosticId": "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2",
              "systemTimestamp": "1996-07-08T09:00:00.000Z"
            })");

  expected_confirmation.user_data.fixed = base::test::ParseJsonDict(
      R"(
          {
            "buildChannel": "release",
            "catalog": [
              {
                "id": "29e5c8bc0ba319069980bb390d8e8f9b58c05a20"
              }
            ],
            "countryCode": "US",
            "createdAtTimestamp": "1996-07-08T09:00:00.000Z",
            "platform": "windows",
            "rotating_hash": "jBdiJH7Hu3wj31WWNLjKV5nVxFxWSDWkYh5zXCS3rXY=",
            "segment": "untargeted",
            "studies": [],
            "topSegment": [],
            "versionNumber": "1.2.3.4"
          })");

  EXPECT_EQ(expected_confirmation, confirmation);
}

TEST_F(BraveAdsRewardConfirmationUtilTest,
       DoNotBuildRewardConfirmationIfNoConfirmationTokens) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*should_use_random_uuids=*/false);

  // Act & Assert
  EXPECT_FALSE(BuildRewardConfirmation(&token_generator_mock_, transaction,
                                       /*user_data=*/{}));
}

TEST_F(BraveAdsRewardConfirmationUtilTest,
       DISABLED_DoNotBuildRewardConfirmationWithInvalidTokenGenerator) {
  // Arrange
  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*should_use_random_uuids=*/false);

  // Act & Assert
  EXPECT_DEATH_IF_SUPPORTED(
      BuildRewardConfirmation(/*token_generator=*/nullptr, transaction,
                              /*user_data=*/{}),
      "Check failed: token_generator");
}

TEST_F(BraveAdsRewardConfirmationUtilTest,
       DISABLED_DoNotBuildRewardConfirmationWithInvalidTransaction) {
  // Arrange
  const TransactionInfo transaction;

  // Act & Assert
  EXPECT_DEATH_IF_SUPPORTED(
      BuildRewardConfirmation(&token_generator_mock_, transaction,
                              /*user_data=*/{}),
      "Check failed: transaction.IsValid*");
}

TEST_F(BraveAdsRewardConfirmationUtilTest,
       DISABLED_DoNotBuildRewardConfirmationForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*should_use_random_uuids=*/false);

  // Act & Assert
  EXPECT_DEATH_IF_SUPPORTED(
      BuildRewardConfirmation(&token_generator_mock_, transaction,
                              /*user_data=*/{}),
      "Check failed: UserHasJoinedBraveRewards*");
}

}  // namespace brave_ads
