/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/database/tables/transactions_database_table.h"

#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/container_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsTransactionsDatabaseTableTest : public UnitTestBase {
 protected:
  BatAdsTransactionsDatabaseTableTest() = default;

  ~BatAdsTransactionsDatabaseTableTest() override = default;
};

TEST_F(BatAdsTransactionsDatabaseTableTest, SaveEmptyTransactions) {
  // Arrange
  const TransactionList& transactions = {};

  // Act
  SaveTransactions(transactions);

  // Assert
  const TransactionList& expected_transactions = transactions;

  database::table::Transactions database_table;
  database_table.GetAll(
      [&expected_transactions](const bool success,
                               const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(CompareAsSets(expected_transactions, transactions));
      });
}

TEST_F(BatAdsTransactionsDatabaseTableTest, SaveTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo& info_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed, DistantFuture());
  transactions.push_back(info_1);

  AdvanceClock(base::Days(5));

  const TransactionInfo& info_2 =
      BuildTransaction(0.03, ConfirmationType::kClicked);
  transactions.push_back(info_2);

  // Act
  SaveTransactions(transactions);

  // Assert
  const TransactionList& expected_transactions = transactions;

  database::table::Transactions database_table;
  database_table.GetAll(
      [&expected_transactions](const bool success,
                               const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(CompareAsSets(expected_transactions, transactions));
      });
}

TEST_F(BatAdsTransactionsDatabaseTableTest, DoNotSaveDuplicateTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo& info =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(info);

  SaveTransactions(transactions);

  // Act
  SaveTransactions(transactions);

  // Assert
  const TransactionList& expected_transactions = transactions;

  database::table::Transactions database_table;
  database_table.GetAll(
      [&expected_transactions](const bool success,
                               const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(CompareAsSets(expected_transactions, transactions));
      });
}

TEST_F(BatAdsTransactionsDatabaseTableTest, GetTransactionsForDateRange) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo& info_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed, DistantFuture());
  transactions.push_back(info_1);

  AdvanceClock(base::Days(5));

  const TransactionInfo& info_2 =
      BuildTransaction(0.03, ConfirmationType::kClicked);
  transactions.push_back(info_2);

  SaveTransactions(transactions);

  // Act
  const TransactionList& expected_transactions = {info_2};

  database::table::Transactions database_table;
  database_table.GetForDateRange(
      Now(), DistantFuture(),
      [&expected_transactions](const bool success,
                               const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(CompareAsSets(expected_transactions, transactions));
      });

  // Assert
}

TEST_F(BatAdsTransactionsDatabaseTableTest, UpdateTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo& info_1 =
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
  database::table::Transactions database_table;
  database_table.Update(unblinded_payment_tokens,
                        [](const bool success) { ASSERT_TRUE(success); });

  // Assert
  info_2.reconciled_at = Now().ToDoubleT();
  const TransactionList& expected_transactions = {info_1, info_2};

  database_table.GetAll(
      [&expected_transactions](const bool success,
                               const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(CompareAsSets(expected_transactions, transactions));
      });
}

TEST_F(BatAdsTransactionsDatabaseTableTest, DeleteTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo& info_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed, DistantFuture());
  transactions.push_back(info_1);

  const TransactionInfo& info_2 =
      BuildTransaction(0.03, ConfirmationType::kClicked);
  transactions.push_back(info_2);

  SaveTransactions(transactions);

  database::table::Transactions database_table;

  // Act
  database_table.Delete([](const bool success) { ASSERT_TRUE(success); });

  // Assert
  database_table.GetAll(
      [](const bool success, const TransactionList& transactions) {
        ASSERT_TRUE(success);
        EXPECT_TRUE(transactions.empty());
      });
}

TEST_F(BatAdsTransactionsDatabaseTableTest, TableName) {
  // Arrange

  // Act
  database::table::Transactions database_table;
  const std::string table_name = database_table.GetTableName();

  // Assert
  const std::string expected_table_name = "transactions";
  EXPECT_EQ(expected_table_name, table_name);
}

}  // namespace ads
