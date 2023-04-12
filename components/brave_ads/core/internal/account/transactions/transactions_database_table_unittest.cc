/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_container_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads::database::table {

class BatAdsTransactionsDatabaseTableTest : public UnitTestBase {};

TEST_F(BatAdsTransactionsDatabaseTableTest, SaveEmptyTransactions) {
  // Arrange

  // Act
  SaveTransactions({});

  // Assert
  const Transactions database_table;
  database_table.GetAll(base::BindOnce(
      [](const bool success, const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(transactions.empty());
      }));
}

TEST_F(BatAdsTransactionsDatabaseTableTest, SaveTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, DistantFuture());
  transactions.push_back(transaction_1);

  AdvanceClockBy(base::Days(5));

  const TransactionInfo transaction_2 =
      BuildTransaction(/*value*/ 0.03, ConfirmationType::kClicked);
  transactions.push_back(transaction_2);

  // Act
  SaveTransactions(transactions);

  // Assert
  const Transactions database_table;
  database_table.GetAll(base::BindOnce(
      [](const TransactionList& expected_transactions, const bool success,
         const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_transactions, transactions));
      },
      std::move(transactions)));
}

TEST_F(BatAdsTransactionsDatabaseTableTest, DoNotSaveDuplicateTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction);

  SaveTransactions(transactions);

  // Act
  SaveTransactions(transactions);

  // Assert
  const Transactions database_table;
  database_table.GetAll(base::BindOnce(
      [](const TransactionList& expected_transactions, const bool success,
         const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_transactions, transactions));
      },
      std::move(transactions)));
}

TEST_F(BatAdsTransactionsDatabaseTableTest, GetTransactionsForDateRange) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, DistantFuture());
  transactions.push_back(transaction_1);

  AdvanceClockBy(base::Days(5));

  const TransactionInfo transaction_2 =
      BuildTransaction(/*value*/ 0.03, ConfirmationType::kClicked);
  transactions.push_back(transaction_2);

  SaveTransactions(transactions);

  // Act
  TransactionList expected_transactions = {transaction_2};

  const Transactions database_table;
  database_table.GetForDateRange(
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

TEST_F(BatAdsTransactionsDatabaseTableTest, UpdateTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, DistantFuture());
  transactions.push_back(transaction_1);

  TransactionInfo transaction_2 =
      BuildTransaction(/*value*/ 0.03, ConfirmationType::kClicked);
  transactions.push_back(transaction_2);

  SaveTransactions(transactions);

  privacy::UnblindedPaymentTokenList unblinded_payment_tokens;
  privacy::UnblindedPaymentTokenInfo unblinded_payment_token;
  unblinded_payment_token.transaction_id = transaction_2.id;
  unblinded_payment_tokens.push_back(unblinded_payment_token);

  // Act
  const Transactions database_table;
  database_table.Update(
      unblinded_payment_tokens,
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  transaction_2.reconciled_at = Now();

  // Assert
  TransactionList expected_transactions = {transaction_1, transaction_2};

  database_table.GetAll(base::BindOnce(
      [](const TransactionList& expected_transactions, const bool success,
         const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_transactions, transactions));
      },
      std::move(expected_transactions)));
}

TEST_F(BatAdsTransactionsDatabaseTableTest, DeleteTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, DistantFuture());
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 =
      BuildTransaction(/*value*/ 0.03, ConfirmationType::kClicked);
  transactions.push_back(transaction_2);

  SaveTransactions(transactions);

  const Transactions database_table;

  // Act
  database_table.Delete(
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  // Assert
  database_table.GetAll(base::BindOnce(
      [](const bool success, const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(transactions.empty());
      }));
}

TEST_F(BatAdsTransactionsDatabaseTableTest, TableName) {
  // Arrange
  const Transactions database_table;

  // Act

  // Assert
  EXPECT_EQ("transactions", database_table.GetTableName());
}

}  // namespace brave_ads::database::table
