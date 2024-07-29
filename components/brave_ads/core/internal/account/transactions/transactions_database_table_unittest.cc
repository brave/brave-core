/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsTransactionsDatabaseTableTest : public test::TestBase {};

TEST_F(BraveAdsTransactionsDatabaseTableTest, SaveEmptyTransactions) {
  // Act
  test::SaveTransactions({});

  // Assert
  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            /*transactions=*/::testing::IsEmpty()));
  const Transactions database_table;
  database_table.GetForDateRange(/*from_time=*/test::DistantPast(),
                                 /*to_time=*/test::DistantFuture(),
                                 callback.Get());
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, SaveTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = test::BuildTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*reconciled_at=*/test::DistantFuture(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_1);

  AdvanceClockBy(base::Days(5));

  const TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.03, AdType::kNotificationAd, ConfirmationType::kClicked,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_2);

  // Act
  test::SaveTransactions(transactions);

  // Assert
  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true, ::testing::ElementsAreArray(transactions)));
  const Transactions database_table;
  database_table.GetForDateRange(/*from_time=*/test::DistantPast(),
                                 /*to_time=*/test::DistantFuture(),
                                 callback.Get());
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, DoNotSaveDuplicateTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction = test::BuildTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction);

  test::SaveTransactions(transactions);

  // Act
  test::SaveTransactions(transactions);

  // Assert
  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, transactions));
  const Transactions database_table;
  database_table.GetForDateRange(/*from_time=*/test::DistantPast(),
                                 /*to_time=*/test::DistantFuture(),
                                 callback.Get());
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, GetTransactionsForDateRange) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = test::BuildTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*reconciled_at=*/test::DistantFuture(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_1);

  AdvanceClockBy(base::Days(5));

  const TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.03, AdType::kNotificationAd, ConfirmationType::kClicked,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_2);

  test::SaveTransactions(transactions);

  const Transactions database_table;

  // Act & Assert
  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, TransactionList{transaction_2}));
  database_table.GetForDateRange(/*from_time=*/test::Now(),
                                 /*to_time=*/test::DistantFuture(),
                                 callback.Get());
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, ReconcileTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = test::BuildTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*reconciled_at=*/test::DistantFuture(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_1);

  TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.03, AdType::kNotificationAd, ConfirmationType::kClicked,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_2);

  test::SaveTransactions(transactions);

  PaymentTokenList payment_tokens;
  PaymentTokenInfo payment_token;
  payment_token.transaction_id = transaction_2.id;
  payment_tokens.push_back(payment_token);

  transaction_2.reconciled_at = test::Now();

  base::MockCallback<ResultCallback> reconcile_callback;
  EXPECT_CALL(reconcile_callback, Run(/*success=*/true));

  const Transactions database_table;

  // Act
  database_table.Reconcile(payment_tokens, reconcile_callback.Get());

  // Assert
  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true,
                  ::testing::UnorderedElementsAreArray(
                      TransactionList{transaction_1, transaction_2})));
  database_table.GetForDateRange(/*from_time=*/test::DistantPast(),
                                 /*to_time=*/test::DistantFuture(),
                                 callback.Get());
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, DeleteTransactions) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = test::BuildTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*reconciled_at=*/test::DistantFuture(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.03, AdType::kNotificationAd, ConfirmationType::kClicked,
      /*should_generate_random_uuids=*/true);
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
  database_table.GetForDateRange(/*from_time=*/test::DistantPast(),
                                 /*to_time=*/test::DistantFuture(),
                                 callback.Get());
}

TEST_F(BraveAdsTransactionsDatabaseTableTest, GetTableName) {
  // Arrange
  const Transactions database_table;

  // Act & Assert
  EXPECT_EQ("transactions", database_table.GetTableName());
}

}  // namespace brave_ads::database::table
