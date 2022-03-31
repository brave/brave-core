/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/transactions/transactions.h"

#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/transaction_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsTransactionsTest : public UnitTestBase {
 protected:
  BatAdsTransactionsTest() = default;

  ~BatAdsTransactionsTest() override = default;
};

TEST_F(BatAdsTransactionsTest, Add) {
  // Arrange

  // Act
  const TransactionInfo& transaction = transactions::Add(
      "42a33833-0a08-4cbb-ab3e-458e020221ab", 0.01, AdType::kAdNotification,
      ConfirmationType::kViewed,
      [](const bool success, const TransactionInfo& transaction) {
        ASSERT_TRUE(success);
      });

  // Assert
  const TransactionList& expected_transactions = {transaction};

  transactions::GetForDateRange(
      DistantPast(), DistantFuture(),
      [&expected_transactions](const bool success,
                               const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_transactions, transactions);
      });
}

TEST_F(BatAdsTransactionsTest, GetForDateRange) {
  // Arrange
  TransactionList transactions;

  AdvanceClock(TimeFromString("31 October 2020", /* is_local */ true));

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  AdvanceClock(TimeFromString("18 November 2020", /* is_local */ true));

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.0, ConfirmationType::kDismissed);
  transactions.push_back(transaction_2);

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.0, ConfirmationType::kClicked);
  transactions.push_back(transaction_3);

  SaveTransactions(transactions);

  // Act
  const TransactionList& expected_transactions = {transaction_2, transaction_3};

  transactions::GetForDateRange(
      Now(), DistantFuture(),
      [&expected_transactions](const bool success,
                               const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_EQ(expected_transactions, transactions);
      });

  // Assert
}

TEST_F(BatAdsTransactionsTest, RemoveAll) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.0, ConfirmationType::kDismissed);
  transactions.push_back(transaction_2);

  SaveTransactions(transactions);

  // Act
  transactions::RemoveAll([](const bool success) { ASSERT_TRUE(success); });

  // Assert
  transactions::GetForDateRange(
      DistantPast(), DistantFuture(),
      [](const bool success, const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(transactions.empty());
      });
}

}  // namespace ads
