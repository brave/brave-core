/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTransactionsTest : public UnitTestBase {};

TEST_F(BraveAdsTransactionsTest, Add) {
  // Arrange

  // Act
  const TransactionInfo transaction =
      AddTransaction("42a33833-0a08-4cbb-ab3e-458e020221ab", "segment", 0.01,
                     AdType::kNotificationAd, ConfirmationType::kViewed,
                     base::BindOnce([](const bool success,
                                       const TransactionInfo& /*transaction*/) {
                       ASSERT_TRUE(success);
                     }));

  // Assert
  TransactionList expected_transactions = {transaction};

  GetTransactionsForDateRange(
      DistantPast(), DistantFuture(),
      base::BindOnce(
          [](const TransactionList& expected_transactions, const bool success,
             const TransactionList& transactions) {
            ASSERT_TRUE(success);
            EXPECT_EQ(expected_transactions, transactions);
          },
          std::move(expected_transactions)));
}

TEST_F(BraveAdsTransactionsTest, GetForDateRange) {
  // Arrange
  TransactionList transactions;

  AdvanceClockTo(TimeFromString("31 October 2020", /*is_local*/ true));

  const TransactionInfo transaction_1 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  AdvanceClockTo(TimeFromString("18 November 2020", /*is_local*/ true));

  const TransactionInfo transaction_2 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.0, ConfirmationType::kDismissed,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  const TransactionInfo transaction_3 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.0, ConfirmationType::kClicked,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_3);

  SaveTransactionsForTesting(transactions);

  // Act
  TransactionList expected_transactions = {transaction_2, transaction_3};

  GetTransactionsForDateRange(
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

TEST_F(BraveAdsTransactionsTest, RemoveAll) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.0, ConfirmationType::kDismissed,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  SaveTransactionsForTesting(transactions);

  // Act
  RemoveAllTransactions(
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  // Assert
  GetTransactionsForDateRange(
      DistantPast(), DistantFuture(),
      base::BindOnce(
          [](const bool success, const TransactionList& transactions) {
            ASSERT_TRUE(success);
            EXPECT_TRUE(transactions.empty());
          }));
}

}  // namespace brave_ads
