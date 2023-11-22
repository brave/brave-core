/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/public/client/ads_client_callback.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsTransactionsDatabaseTableTest : public UnitTestBase {};

TEST_F(BraveAdsTransactionsDatabaseTableTest, SaveEmptyTransactions) {
  // Act
  test::SaveTransactions({});

  // Assert
  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            /*transactions=*/::testing::IsEmpty()));
  const Transactions database_table;
  database_table.GetAll(callback.Get());
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, SaveTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = test::BuildTransaction(
      /*value=*/0.01, ConfirmationType::kViewed, DistantFuture(),
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_1);

  AdvanceClockBy(base::Days(5));

  const TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.03, ConfirmationType::kClicked,
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_2);

  // Act
  test::SaveTransactions(transactions);

  // Assert
  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true,
                  ::testing::UnorderedElementsAreArray(transactions)));
  const Transactions database_table;
  database_table.GetAll(callback.Get());
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, DoNotSaveDuplicateTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction = test::BuildTransaction(
      /*value=*/0.01, ConfirmationType::kViewed, /*reconciled_at=*/Now(),
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction);

  test::SaveTransactions(transactions);

  // Act
  test::SaveTransactions(transactions);

  // Assert
  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true,
                  ::testing::UnorderedElementsAreArray(transactions)));
  const Transactions database_table;
  database_table.GetAll(callback.Get());
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, GetTransactionsForDateRange) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = test::BuildTransaction(
      /*value=*/0.01, ConfirmationType::kViewed, DistantFuture(),
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_1);

  AdvanceClockBy(base::Days(5));

  const TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.03, ConfirmationType::kClicked,
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_2);

  test::SaveTransactions(transactions);

  const Transactions database_table;

  // Act & Assert
  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, TransactionList{transaction_2}));
  database_table.GetForDateRange(Now(), DistantFuture(), callback.Get());
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, UpdateTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = test::BuildTransaction(
      /*value=*/0.01, ConfirmationType::kViewed, DistantFuture(),
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_1);

  TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.03, ConfirmationType::kClicked,
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_2);

  test::SaveTransactions(transactions);

  PaymentTokenList payment_tokens;
  PaymentTokenInfo payment_token;
  payment_token.transaction_id = transaction_2.id;
  payment_tokens.push_back(payment_token);

  transaction_2.reconciled_at = Now();

  base::MockCallback<ResultCallback> update_callback;
  EXPECT_CALL(update_callback, Run(/*success=*/true));

  const Transactions database_table;

  // Act
  database_table.Update(payment_tokens, update_callback.Get());

  // Assert
  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true,
                  ::testing::UnorderedElementsAreArray(
                      TransactionList{{transaction_1, transaction_2}})));
  database_table.GetAll(callback.Get());
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, DeleteTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = test::BuildTransaction(
      /*value=*/0.01, ConfirmationType::kViewed, DistantFuture(),
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.03, ConfirmationType::kClicked,
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_2);

  test::SaveTransactions(transactions);

  base::MockCallback<ResultCallback> delete_callback;
  EXPECT_CALL(delete_callback, Run(/*success=*/true));

  const Transactions database_table;

  // Act
  database_table.Delete(delete_callback.Get());

  // Assert
  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            /*transactions=*/::testing::IsEmpty()));
  database_table.GetAll(callback.Get());
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, GetTableName) {
  // Arrange
  const Transactions database_table;

  // Act & Assert
  EXPECT_EQ("transactions", database_table.GetTableName());
}

}  // namespace brave_ads::database::table
