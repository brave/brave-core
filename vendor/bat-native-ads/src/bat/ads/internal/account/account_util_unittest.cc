/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/account_util.h"

#include "base/functional/bind.h"
#include "bat/ads/internal/account/confirmations/confirmation_unittest_util.h"
#include "bat/ads/internal/account/transactions/transactions_database_table.h"
#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"
#include "bat/ads/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"
#include "brave/components/brave_ads/common/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsAccountUtilTest : public UnitTestBase {};

TEST_F(BatAdsAccountUtilTest, ShouldRewardUser) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, true);

  // Act
  const bool should_reward_user = ShouldRewardUser();

  // Assert
  EXPECT_TRUE(should_reward_user);
}

TEST_F(BatAdsAccountUtilTest, ShouldNotRewardUser) {
  // Arrange
  AdsClientHelper::GetInstance()->SetBooleanPref(prefs::kEnabled, false);

  // Act
  const bool should_reward_user = ShouldRewardUser();

  // Assert
  EXPECT_FALSE(should_reward_user);
}

TEST_F(BatAdsAccountUtilTest, ResetRewards) {
  // Arrange
  TransactionList transactions;
  const TransactionInfo transaction =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction);
  SaveTransactions(transactions);

  privacy::SetUnblindedTokens(1);
  const absl::optional<ConfirmationInfo> confirmation = BuildConfirmation();
  ASSERT_TRUE(confirmation);
  ConfirmationStateManager::GetInstance()->AppendFailedConfirmation(
      *confirmation);

  const privacy::UnblindedPaymentTokenList unblinded_payment_tokens =
      privacy::GetUnblindedPaymentTokens(1);
  privacy::GetUnblindedPaymentTokens()->AddTokens(unblinded_payment_tokens);

  // Act
  ResetRewards(base::BindOnce([](const bool success) {
    ASSERT_TRUE(success);

    const database::table::Transactions database_table;
    database_table.GetAll(base::BindOnce(
        [](const bool success, const TransactionList& transactions) {
          ASSERT_TRUE(success);
          EXPECT_TRUE(transactions.empty());
        }));

    const ConfirmationList& failed_confirmations =
        ConfirmationStateManager::GetInstance()->GetFailedConfirmations();
    EXPECT_TRUE(failed_confirmations.empty());

    EXPECT_TRUE(privacy::UnblindedPaymentTokensIsEmpty());
  }));

  // Assert
}

TEST_F(BatAdsAccountUtilTest, ResetRewardsWithNoState) {
  // Arrange

  // Act
  ResetRewards(base::BindOnce([](const bool success) {
    ASSERT_TRUE(success);

    const database::table::Transactions database_table;
    database_table.GetAll(base::BindOnce(
        [](const bool success, const TransactionList& transactions) {
          ASSERT_TRUE(success);
          EXPECT_TRUE(transactions.empty());
        }));

    const ConfirmationList& failed_confirmations =
        ConfirmationStateManager::GetInstance()->GetFailedConfirmations();
    EXPECT_TRUE(failed_confirmations.empty());

    EXPECT_TRUE(privacy::UnblindedPaymentTokensIsEmpty());
  }));

  // Assert
}

}  // namespace ads
