/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/transactions/transactions_database_table.h"

#include <utility>

#include "base/functional/bind.h"
#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_container_util.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::database::table {

class BatAdsTransactionsDatabaseTableTest : public UnitTestBase {};

TEST_F(BatAdsTransactionsDatabaseTableTest, SaveEmptyTransactions) {
  // Arrange
  const TransactionList transactions;

  // Act
  SaveTransactions(transactions);

  // Assert
  TransactionList expected_transactions = transactions;

  const Transactions database_table;
  database_table.GetAll(base::BindOnce(
      [](const TransactionList& expected_transactions, const bool success,
         const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_transactions, transactions));
      },
      std::move(expected_transactions)));
}

TEST_F(BatAdsTransactionsDatabaseTableTest, SaveTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo info_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed, DistantFuture());
  transactions.push_back(info_1);

  AdvanceClockBy(base::Days(5));

  const TransactionInfo info_2 =
      BuildTransaction(0.03, ConfirmationType::kClicked);
  transactions.push_back(info_2);

  // Act
  SaveTransactions(transactions);

  // Assert
  TransactionList expected_transactions = transactions;

  const Transactions database_table;
  database_table.GetAll(base::BindOnce(
      [](const TransactionList& expected_transactions, const bool success,
         const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_transactions, transactions));
      },
      std::move(expected_transactions)));
}

TEST_F(BatAdsTransactionsDatabaseTableTest, DoNotSaveDuplicateTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo info =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(info);

  SaveTransactions(transactions);

  // Act
  SaveTransactions(transactions);

  // Assert
  TransactionList expected_transactions = transactions;

  const Transactions database_table;
  database_table.GetAll(base::BindOnce(
      [](const TransactionList& expected_transactions, const bool success,
         const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(ContainersEq(expected_transactions, transactions));
      },
      std::move(expected_transactions)));
}

TEST_F(BatAdsTransactionsDatabaseTableTest, GetTransactionsForDateRange) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo info_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed, DistantFuture());
  transactions.push_back(info_1);

  AdvanceClockBy(base::Days(5));

  const TransactionInfo info_2 =
      BuildTransaction(0.03, ConfirmationType::kClicked);
  transactions.push_back(info_2);

  SaveTransactions(transactions);

  // Act
  TransactionList expected_transactions = {info_2};

  const Transactions database_table;
  database_table.GetForDateRange(
      Now(), DistantFuture(),
      base::BindOnce(
          [](const TransactionList& expected_transactions, const bool success,
             const TransactionList& transactions) {
            ASSERT_TRUE(success);
            EXPECT_TRUE(ContainersEq(expected_transactions, transactions));
          },
          std::move(expected_transactions)));

  // Assert
}

TEST_F(BatAdsTransactionsDatabaseTableTest, UpdateTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo info_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed, DistantFuture());
  transactions.push_back(info_1);

  TransactionInfo info_2 = BuildTransaction(0.03, ConfirmationType::kClicked);
  transactions.push_back(info_2);

  SaveTransactions(transactions);

  privacy::UnblindedPaymentTokenList unblinded_payment_tokens;
  privacy::UnblindedPaymentTokenInfo unblinded_payment_token;
  unblinded_payment_token.transaction_id = info_2.id;
  unblinded_payment_tokens.push_back(unblinded_payment_token);

  // Act
  const Transactions database_table;
  database_table.Update(
      unblinded_payment_tokens,
      base::BindOnce([](const bool success) { ASSERT_TRUE(success); }));

  // Assert
  info_2.reconciled_at = Now();
  TransactionList expected_transactions = {info_1, info_2};

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

  const TransactionInfo info_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed, DistantFuture());
  transactions.push_back(info_1);

  const TransactionInfo info_2 =
      BuildTransaction(0.03, ConfirmationType::kClicked);
  transactions.push_back(info_2);

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
  const std::string table_name = database_table.GetTableName();

  // Assert
  const std::string expected_table_name = "transactions";
  EXPECT_EQ(expected_table_name, table_name);
}

}  // namespace ads::database::table
