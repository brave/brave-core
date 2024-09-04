/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"

#include "base/test/mock_callback.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_callback.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::database::table {

class BraveAdsTransactionsDatabaseTableTest : public test::TestBase {};

TEST_F(BraveAdsTransactionsDatabaseTableTest, SaveEmptyTransactions) {
  // Act
  database::SaveTransactions({});

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
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*reconciled_at=*/test::DistantFuture(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_1);

  AdvanceClockBy(base::Days(5));

  const TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.03, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kClicked,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_2);

  // Act
  database::SaveTransactions(transactions);

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
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction);

  database::SaveTransactions(transactions);

  // Act
  database::SaveTransactions(transactions);

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
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*reconciled_at=*/test::DistantFuture(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_1);

  AdvanceClockBy(base::Days(5));

  const TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.03, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kClicked,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_2);

  database::SaveTransactions(transactions);

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
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*reconciled_at=*/test::DistantFuture(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_1);

  TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.03, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kClicked,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_2);

  database::SaveTransactions(transactions);

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

TEST_F(BraveAdsTransactionsDatabaseTableTest, PurgeExpired) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("Tue, 19 Mar 2024 05:35"));

  TransactionList transactions;

  const TransactionInfo transaction_1 = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*reconciled_at=*/test::DistantFuture(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_1);

  AdvanceClockBy(base::Days(90));

  const TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.03, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kClicked,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_2);

  const TransactionInfo transaction_3 = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*reconciled_at=*/test::DistantFuture(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_3);

  database::SaveTransactions(transactions);

  const Transactions database_table;

  // Act & Assert
  base::MockCallback<ResultCallback> purge_expired_callback;
  EXPECT_CALL(purge_expired_callback, Run(/*success=*/true));
  database_table.PurgeExpired(purge_expired_callback.Get());

  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true,
                  ::testing::UnorderedElementsAreArray(
                      TransactionList{transaction_2, transaction_3})));
  database_table.GetForDateRange(/*from_time=*/test::DistantPast(),
                                 /*to_time=*/test::DistantFuture(),
                                 callback.Get());
}

TEST_F(BraveAdsTransactionsDatabaseTableTest,
       DoNotPurgeExpiredOnTheCuspOfExpiration) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("Tue, 19 Mar 2024 05:35"));

  TransactionList transactions;

  const TransactionInfo transaction_1 = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*reconciled_at=*/test::DistantFuture(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_1);

  AdvanceClockBy(base::Days(90) - base::Milliseconds(1));

  const TransactionInfo transaction_2 = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kClicked,
      /*reconciled_at=*/test::DistantFuture(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_2);

  database::SaveTransactions(transactions);

  const Transactions database_table;

  // Act & Assert
  base::MockCallback<ResultCallback> purge_expired_callback;
  EXPECT_CALL(purge_expired_callback, Run(/*success=*/true));
  database_table.PurgeExpired(purge_expired_callback.Get());

  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback,
              Run(/*success=*/true,
                  ::testing::UnorderedElementsAreArray(
                      TransactionList{transaction_1, transaction_2})));
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
