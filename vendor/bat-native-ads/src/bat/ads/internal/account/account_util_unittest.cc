/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/account_util.h"

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/account/confirmations/confirmations_state.h"
#include "bat/ads/internal/account/confirmations/confirmations_unittest_util.h"
#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/database/tables/transactions_database_table.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_tokens.h"
#include "bat/ads/internal/privacy/unblinded_payment_tokens/unblinded_payment_tokens_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsAccountUtilTest : public UnitTestBase {
 protected:
  BatAdsAccountUtilTest() = default;

  ~BatAdsAccountUtilTest() override = default;
};

TEST_F(BatAdsAccountUtilTest, ShouldRewardUser) {
  // Arrange
  AdsClientHelper::Get()->SetBooleanPref(prefs::kEnabled, true);

  // Act
  const bool should_reward_user = ShouldRewardUser();

  // Assert
  EXPECT_TRUE(should_reward_user);
}

TEST_F(BatAdsAccountUtilTest, ShouldNotRewardUser) {
  // Arrange
  AdsClientHelper::Get()->SetBooleanPref(prefs::kEnabled, false);

  // Act
  const bool should_reward_user = ShouldRewardUser();

  // Assert
  EXPECT_FALSE(should_reward_user);
}

TEST_F(BatAdsAccountUtilTest, ResetRewards) {
  // Arrange
  TransactionList transactions;
  const TransactionInfo& transaction =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction);
  SaveTransactions(transactions);

  const ConfirmationInfo& confirmation =
      BuildConfirmation("d990ed8d-d739-49fb-811b-c2e02158fb60",
                        "8b742869-6e4a-490c-ac31-31b49130098a",
                        "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
                        ConfirmationType::kViewed, AdType::kAdNotification);
  ConfirmationsState::Get()->AppendFailedConfirmation(confirmation);

  const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens =
      privacy::GetRandomUnblindedPaymentTokens(1);
  privacy::get_unblinded_payment_tokens()->AddTokens(unblinded_payment_tokens);

  // Act
  ResetRewards([](const bool success) {
    ASSERT_TRUE(success);

    database::table::Transactions database_table;
    database_table.GetAll(
        [=](const bool success, const TransactionList& transactions) {
          ASSERT_TRUE(success);
          EXPECT_TRUE(transactions.empty());
        });

    const ConfirmationList& confirmations =
        ConfirmationsState::Get()->GetFailedConfirmations();
    EXPECT_TRUE(confirmations.empty());

    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens =
        privacy::get_unblinded_payment_tokens()->GetAllTokens();
    EXPECT_TRUE(unblinded_payment_tokens.empty());
  });

  // Assert
}

TEST_F(BatAdsAccountUtilTest, ResetRewardsWithNoState) {
  // Arrange

  // Act
  ResetRewards([](const bool success) {
    ASSERT_TRUE(success);

    database::table::Transactions database_table;
    database_table.GetAll(
        [=](const bool success, const TransactionList& transactions) {
          ASSERT_TRUE(success);
          EXPECT_TRUE(transactions.empty());
        });

    const ConfirmationList& confirmations =
        ConfirmationsState::Get()->GetFailedConfirmations();
    EXPECT_TRUE(confirmations.empty());

    const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens =
        privacy::get_unblinded_payment_tokens()->GetAllTokens();
    EXPECT_TRUE(unblinded_payment_tokens.empty());
  });

  // Assert
}

}  // namespace ads
