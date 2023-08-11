/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"

#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/confirmation_tokens/confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::NiceMock;

class BraveAdsRewardConfirmationsUtilTest : public UnitTestBase {
 protected:
  NiceMock<privacy::TokenGeneratorMock> token_generator_mock_;
};

TEST_F(BraveAdsRewardConfirmationsUtilTest, BuildRewardCredential) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  privacy::SetConfirmationTokensForTesting(/*count*/ 1);

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.1, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  const absl::optional<ConfirmationInfo> confirmation = BuildRewardConfirmation(
      &token_generator_mock_, transaction, /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  // Act

  // Assert
  EXPECT_TRUE(BuildRewardCredential(*confirmation));
}

TEST_F(BraveAdsRewardConfirmationsUtilTest, BuildRewardConfirmation) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  privacy::SetConfirmationTokensForTesting(/*count*/ 1);

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.1, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);

  // Act
  const absl::optional<ConfirmationInfo> confirmation =
      BuildRewardConfirmation(&token_generator_mock_, transaction,
                              /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  // Assert
  // TODO(tmancey): Check data structure values.
  EXPECT_TRUE(confirmation->reward);
  EXPECT_TRUE(IsValid(*confirmation));
}

TEST_F(BraveAdsRewardConfirmationsUtilTest,
       DoNotBuildRewardConfirmationIfNoConfirmationTokens) {
  // Arrange
  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.1, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);

  // Act

  // Assert
  EXPECT_FALSE(BuildRewardConfirmation(&token_generator_mock_, transaction,
                                       /*user_data*/ {}));
}

}  // namespace brave_ads
