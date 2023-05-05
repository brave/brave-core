/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/account_util.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/unblinded_tokens/unblinded_tokens_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::NiceMock;

class BraveAdsAccountUtilTest : public UnitTestBase {
 protected:
  NiceMock<privacy::TokenGeneratorMock> token_generator_mock_;
};

TEST_F(BraveAdsAccountUtilTest, ShouldRewardUser) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(ShouldRewardUser());
}

TEST_F(BraveAdsAccountUtilTest, ShouldNotRewardUser) {
  // Arrange
  ads_client_mock_.SetBooleanPref(prefs::kEnabled, false);

  // Act

  // Assert
  EXPECT_FALSE(ShouldRewardUser());
}

TEST_F(BraveAdsAccountUtilTest, ResetRewards) {
  // Arrange
  TransactionList transactions;
  const TransactionInfo transaction =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction);
  SaveTransactions(transactions);

  MockTokenGenerator(token_generator_mock_, /*count*/ 1);

  privacy::SetUnblindedTokens(/*count*/ 1);
  privacy::SetUnblindedPaymentTokens(/*count*/ 1);

  const absl::optional<ConfirmationInfo> confirmation =
      BuildConfirmation(&token_generator_mock_, transaction);
  ASSERT_TRUE(confirmation);
  ConfirmationStateManager::GetInstance().AppendFailedConfirmation(
      *confirmation);

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
        ConfirmationStateManager::GetInstance().GetFailedConfirmations();
    EXPECT_TRUE(failed_confirmations.empty());

    EXPECT_TRUE(privacy::UnblindedPaymentTokensIsEmpty());
  }));

  // Assert
}

TEST_F(BraveAdsAccountUtilTest, ResetRewardsWithNoState) {
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
        ConfirmationStateManager::GetInstance().GetFailedConfirmations();
    EXPECT_TRUE(failed_confirmations.empty());

    EXPECT_TRUE(privacy::UnblindedPaymentTokensIsEmpty());
  }));

  // Assert
}

}  // namespace brave_ads
