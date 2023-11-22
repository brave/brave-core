/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"

#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationsUtilTest : public UnitTestBase {
 protected:
  TokenGeneratorMock token_generator_mock_;
};

TEST_F(BraveAdsConfirmationsUtilTest, IsInvalidToken) {
  // Arrange
  const ConfirmationInfo confirmation;

  // Act & Assert
  EXPECT_FALSE(IsValid(confirmation));
}

TEST_F(BraveAdsConfirmationsUtilTest, ResetTokens) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::SetConfirmationTokens(/*count=*/2);

  test::SetPaymentTokens(/*count=*/1);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/true);
  const absl::optional<ConfirmationInfo> confirmation =
      BuildRewardConfirmation(&token_generator_mock_, transaction,
                              /*user_data=*/{});
  ASSERT_TRUE(confirmation);
  ConfirmationStateManager::GetInstance().AddConfirmation(*confirmation);

  // Act
  ResetTokens();

  // Assert
  const ConfirmationList& confirmations =
      ConfirmationStateManager::GetInstance().GetConfirmations();
  EXPECT_TRUE(confirmations.empty());

  EXPECT_TRUE(ConfirmationTokensIsEmpty());

  EXPECT_TRUE(PaymentTokensIsEmpty());
}

TEST_F(BraveAdsConfirmationsUtilTest, ResetIfNoTokens) {
  // Act
  ResetTokens();

  // Assert
  const ConfirmationList& confirmations =
      ConfirmationStateManager::GetInstance().GetConfirmations();
  EXPECT_TRUE(confirmations.empty());

  EXPECT_TRUE(ConfirmationTokensIsEmpty());

  EXPECT_TRUE(PaymentTokensIsEmpty());
}

}  // namespace brave_ads
