/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"

#include <utility>

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/ads_client_callback.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_container_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

namespace {

void ExpectTransactionsEq(const TransactionList expected_transactions) {
  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([&expected_transactions](const bool success,
                                         const TransactionList& transactions) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_transactions, transactions));
      });

  const Transactions database_table;
  database_table.GetAll(callback.Get());
}

}  // namespace

class BraveAdsTransactionsDatabaseTableTest : public UnitTestBase {};

TEST_F(BraveAdsTransactionsDatabaseTableTest, SaveEmptyTransactions) {
  // Arrange

  // Act
  SaveTransactionsForTesting({});

  // Assert
  ExpectTransactionsEq({});
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, SaveTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = BuildTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed, DistantFuture(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  AdvanceClockBy(base::Days(5));

  const TransactionInfo transaction_2 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.03, ConfirmationType::kClicked,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  // Act
  SaveTransactionsForTesting(transactions);

  // Assert
  ExpectTransactionsEq(transactions);
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, DoNotSaveDuplicateTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction = BuildTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction);

  SaveTransactionsForTesting(transactions);

  // Act
  SaveTransactionsForTesting(transactions);

  // Assert
  ExpectTransactionsEq(transactions);
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, GetTransactionsForDateRange) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = BuildTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed, DistantFuture(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  AdvanceClockBy(base::Days(5));

  const TransactionInfo transaction_2 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.03, ConfirmationType::kClicked,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  SaveTransactionsForTesting(transactions);

  // Assert
  const TransactionList expected_transactions = {transaction_2};
  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([&expected_transactions](const bool success,
                                         const TransactionList& transactions) {
        EXPECT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_transactions, transactions));
      });

  // Act
  const Transactions database_table;
  database_table.GetForDateRange(Now(), DistantFuture(), callback.Get());
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, UpdateTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = BuildTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed, DistantFuture(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  TransactionInfo transaction_2 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.03, ConfirmationType::kClicked,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  SaveTransactionsForTesting(transactions);

  PaymentTokenList payment_tokens;
  PaymentTokenInfo payment_token;
  payment_token.transaction_id = transaction_2.id;
  payment_tokens.push_back(payment_token);

  // Assert
  transaction_2.reconciled_at = Now();
  const TransactionList expected_transactions = {transaction_1, transaction_2};
  base::MockCallback<ResultCallback> callback;
  EXPECT_CALL(callback, Run)
      .WillOnce([&expected_transactions](const bool success) {
        EXPECT_TRUE(success);
        ExpectTransactionsEq(expected_transactions);
      });

  // Act
  const Transactions database_table;
  database_table.Update(payment_tokens, callback.Get());
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, DeleteTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = BuildTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed, DistantFuture(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.03, ConfirmationType::kClicked,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  SaveTransactionsForTesting(transactions);

  // Assert
  base::MockCallback<ResultCallback> callback;
  EXPECT_CALL(callback, Run).WillOnce([](const bool success) {
    EXPECT_TRUE(success);
    ExpectTransactionsEq({});
  });

  // Act
  const Transactions database_table;
  database_table.Delete(callback.Get());
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, TableName) {
  // Arrange
  const Transactions database_table;

  // Act

  // Assert
  EXPECT_EQ("transactions", database_table.GetTableName());
}

}  // namespace brave_ads::database::table
