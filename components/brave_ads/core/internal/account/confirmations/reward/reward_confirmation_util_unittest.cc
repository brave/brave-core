/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"

#include "base/strings/string_util.h"
#include "base/test/mock_callback.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/browser/browser_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsRewardConfirmationUtilTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    MockConfirmationUserData();

    AdvanceClockTo(
        TimeFromString("Mon, 8 Jul 1996 09:25:00", /*is_local*/ false));
  }

  ::testing::NiceMock<TokenGeneratorMock> token_generator_mock_;
};

TEST_F(BraveAdsRewardConfirmationUtilTest, BuildRewardCredential) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  SetConfirmationTokensForTesting(/*count*/ 1);

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ false);
  const absl::optional<ConfirmationInfo> confirmation = BuildRewardConfirmation(
      &token_generator_mock_, transaction, /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  // Act

  // Assert
  EXPECT_EQ(
      R"(eyJzaWduYXR1cmUiOiJrM3hJalZwc0FYTGNHL0NKRGVLQVphN0g3aGlrMVpyUThIOVpEZC9KVU1SQWdtYk5WY0V6VnhRb2dDZDBjcmlDZnZCQWtsd1hybWNyeVBaaFUxMlg3Zz09IiwidCI6IlBMb3d6MldGMmVHRDV6Zndaams5cDc2SFhCTERLTXEvM0VBWkhlRy9mRTJYR1E0OGp5dGUrVmU1MFpsYXNPdVlMNW13QThDVTJhRk1sSnJ0M0REZ0N3PT0ifQ==)",
      BuildRewardCredential(*confirmation));
}

TEST_F(BraveAdsRewardConfirmationUtilTest, BuildRewardConfirmation) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  SetConfirmationTokensForTesting(/*count*/ 1);

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ false);

  // Assert
  base::MockCallback<BuildConfirmationUserDataCallback> callback;
  EXPECT_CALL(callback, Run).WillOnce([=](const UserDataInfo& user_data) {
    const absl::optional<ConfirmationInfo> confirmation =
        BuildRewardConfirmation(&token_generator_mock_, transaction, user_data);
    ASSERT_TRUE(confirmation);

    ConfirmationInfo expected_confirmation;

    expected_confirmation.transaction_id = kTransactionId;
    expected_confirmation.creative_instance_id = kCreativeInstanceId;
    expected_confirmation.type = ConfirmationType::kViewed;
    expected_confirmation.ad_type = AdType::kNotificationAd;
    expected_confirmation.created_at = Now();
    expected_confirmation.was_created = false;

    expected_confirmation.reward = BuildRewardForTesting(*confirmation);

    expected_confirmation.user_data.dynamic = base::test::ParseJsonDict(
        R"({"diagnosticId":"c1298fde-7fdb-401f-a3ce-0b58fe86e6e2","systemTimestamp":"1996-07-08T09:00:00.000Z"})");

    const std::string expected_fixed_data = base::ReplaceStringPlaceholders(
        R"({"buildChannel":"release","catalog":[{"id":"29e5c8bc0ba319069980bb390d8e8f9b58c05a20"}],"countryCode":"US","createdAtTimestamp":"1996-07-08T09:00:00.000Z","platform":"windows","rotating_hash":"jBdiJH7Hu3wj31WWNLjKV5nVxFxWSDWkYh5zXCS3rXY=","segment":"untargeted","studies":[],"versionNumber":"$1"})",
        {GetBrowserVersionNumber()}, nullptr);
    expected_confirmation.user_data.fixed =
        base::test::ParseJsonDict(expected_fixed_data);
    ASSERT_TRUE(IsValid(expected_confirmation));

    EXPECT_EQ(expected_confirmation, confirmation);
  });

  // Act
  BuildConfirmationUserData(transaction, callback.Get());
}

TEST_F(BraveAdsRewardConfirmationUtilTest,
       DoNotBuildRewardConfirmationIfNoConfirmationTokens) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);

  // Act

  // Assert
  EXPECT_FALSE(BuildRewardConfirmation(&token_generator_mock_, transaction,
                                       /*user_data*/ {}));
}

}  // namespace brave_ads
