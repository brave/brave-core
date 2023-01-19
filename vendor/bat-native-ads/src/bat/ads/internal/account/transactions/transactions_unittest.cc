/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/transactions/transactions.h"

#include <utility>

#include "base/functional/bind.h"
#include "bat/ads/internal/account/transactions/transaction_info.h"
#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsTransactionsTest : public UnitTestBase {};

TEST_F(BatAdsTransactionsTest, Add) {
  // Arrange

  // Act
  const TransactionInfo transaction = transactions::Add(
      "42a33833-0a08-4cbb-ab3e-458e020221ab", 0.01, AdType::kNotificationAd,
      ConfirmationType::kViewed,
      base::BindOnce(
          [](const bool success, const TransactionInfo& /*transaction*/) {
            ASSERT_TRUE(success);
          }));

  // Assert
  TransactionList expected_transactions = {transaction};

  transactions::GetForDateRange(
      DistantPast(), DistantFuture(),
      base::BindOnce(
          [](const TransactionList& expected_transactions, const bool success,
             const TransactionList& transactions) {
            ASSERT_TRUE(success);
            EXPECT_EQ(expected_transactions, transactions);
          },
          std::move(expected_transactions)));
}

TEST_F(BatAdsTransactionsTest, GetForDateRange) {
  // Arrange
  TransactionList transactions;

  AdvanceClockTo(TimeFromString("31 October 2020", /*is_local*/ true));

  const TransactionInfo transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  AdvanceClockTo(TimeFromString("18 November 2020", /*is_local*/ true));

  const TransactionInfo transaction_2 =
      BuildTransaction(0.0, ConfirmationType::kDismissed);
  transactions.push_back(transaction_2);

  const TransactionInfo transaction_3 =
      BuildTransaction(0.0, ConfirmationType::kClicked);
  transactions.push_back(transaction_3);

  SaveTransactions(transactions);

  // Act
  TransactionList expected_transactions = {transaction_2, transaction_3};

  transactions::GetForDateRange(
      Now(), DistantFuture(),
      base::BindOnce(
          [](const TransactionList& expected_transactions, const bool success,
             const TransactionList& transactions) {
            ASSERT_TRUE(success);
            EXPECT_EQ(expected_transactions, transactions);
          },
          std::move(expected_transactions)));

  // Assert
}

TEST_F(BatAdsTransactionsTest, RemoveAll) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 =
      BuildTransaction(0.0, ConfirmationType::kDismissed);
  transactions.push_back(transaction_2);

  SaveTransactions(transactions);

  // Act
  transactions::RemoveAll(
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  // Assert
  transactions::GetForDateRange(
      DistantPast(), DistantFuture(),
      base::BindOnce(
          [](const bool success, const TransactionList& transactions) {
            ASSERT_TRUE(success);
            EXPECT_TRUE(transactions.empty());
          }));
}

}  // namespace ads
