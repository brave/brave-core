/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_util.h"

#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_token_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;

class BraveAdsConfirmationUtilTest : public UnitTestBase {
 protected:
  NiceMock<privacy::TokenGeneratorMock> token_generator_mock_;
};

TEST_F(BraveAdsConfirmationUtilTest, CreateConfirmationForNonOptedInUser) {
  // Arrange
  ads_client_mock_.SetBooleanPref(prefs::kEnabled, false);

  privacy::SetUnblindedTokens(/*count*/ 1);

  ON_CALL(token_generator_mock_, Generate(_))
      .WillByDefault(Return(privacy::GetTokens(/*count*/ 1)));

  const TransactionInfo transaction =
      BuildTransaction(/*value*/ 0.0, ConfirmationType::kViewed);

  // Act
  const absl::optional<ConfirmationInfo> confirmation =
      CreateConfirmation(&token_generator_mock_, transaction,
                         /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  // Assert
  EXPECT_FALSE(confirmation->opted_in);
  EXPECT_TRUE(IsValid(*confirmation));
}

TEST_F(BraveAdsConfirmationUtilTest, IsNotValidForNonOptedInUser) {
  // Arrange
  ads_client_mock_.SetBooleanPref(prefs::kEnabled, false);

  // Act
  const ConfirmationInfo confirmation;

  // Assert
  EXPECT_FALSE(IsValid(confirmation));
}

TEST_F(BraveAdsConfirmationUtilTest, CreateConfirmationForOptedInUser) {
  // Arrange
  privacy::SetUnblindedTokens(/*count*/ 1);

  ON_CALL(token_generator_mock_, Generate(_))
      .WillByDefault(Return(privacy::GetTokens(/*count*/ 1)));

  const TransactionInfo transaction =
      BuildTransaction(/*value*/ 0.0, ConfirmationType::kViewed);

  // Act
  const absl::optional<ConfirmationInfo> confirmation =
      CreateConfirmation(&token_generator_mock_, transaction,
                         /*user_data*/ {});
  ASSERT_TRUE(confirmation);

  // Assert
  EXPECT_TRUE(confirmation->opted_in);
  EXPECT_TRUE(IsValid(*confirmation));
}

TEST_F(BraveAdsConfirmationUtilTest, FailToCreateConfirmationForOptedInUser) {
  // Arrange
  ON_CALL(token_generator_mock_, Generate(_))
      .WillByDefault(Return(privacy::GetTokens(/*count*/ 1)));

  const TransactionInfo transaction =
      BuildTransaction(/*value*/ 0.0, ConfirmationType::kViewed);

  // Act
  const absl::optional<ConfirmationInfo> confirmation =
      CreateConfirmation(&token_generator_mock_, transaction,
                         /*user_data*/ {});

  // Assert
  EXPECT_FALSE(confirmation);
}

TEST_F(BraveAdsConfirmationUtilTest, IsNotValidForOptedInUser) {
  // Arrange

  // Act
  const ConfirmationInfo confirmation;

  // Assert
  EXPECT_FALSE(IsValid(confirmation));
}

TEST_F(BraveAdsConfirmationUtilTest, ResetConfirmations) {
  // Arrange
  privacy::SetUnblindedTokens(/*count*/ 2);

  privacy::SetUnblindedPaymentTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);
  ConfirmationStateManager::GetInstance().AppendFailedConfirmation(
      *confirmation);

  // Act
  ResetConfirmations();

  // Assert
  const ConfirmationList& failed_confirmations =
      ConfirmationStateManager::GetInstance().GetFailedConfirmations();
  EXPECT_TRUE(failed_confirmations.empty());

  EXPECT_TRUE(privacy::UnblindedPaymentTokensIsEmpty());

  EXPECT_TRUE(privacy::UnblindedTokensIsEmpty());
}

TEST_F(BraveAdsConfirmationUtilTest, ResetEmptyConfirmations) {
  // Arrange

  // Act
  ResetConfirmations();

  // Assert
  const ConfirmationList& failed_confirmations =
      ConfirmationStateManager::GetInstance().GetFailedConfirmations();
  EXPECT_TRUE(failed_confirmations.empty());

  EXPECT_TRUE(privacy::UnblindedPaymentTokensIsEmpty());

  EXPECT_TRUE(privacy::UnblindedTokensIsEmpty());
}

}  // namespace brave_ads
