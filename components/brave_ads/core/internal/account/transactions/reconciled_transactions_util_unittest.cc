/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/reconciled_transactions_util.h"

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsReconciledTransactionsUtilTest : public UnitTestBase {};

TEST_F(BraveAdsReconciledTransactionsUtilTest,
       DidReconcileTransactionsThisMonth) {
  // Arrange
  AdvanceClockTo(TimeFromString("Wed, 16 Sep 2015 23:01",
                                /*is_local=*/true));  // Hello Millie!!!

  TransactionList transactions;
  const TransactionInfo transaction = BuildTransactionForTesting(
      /*value=*/0.01, ConfirmationType::kViewed, /*reconciled_at=*/Now(),
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction);

  // Act & Assert
  EXPECT_TRUE(DidReconcileTransactionsThisMonth(transactions));
}

TEST_F(BraveAdsReconciledTransactionsUtilTest,
       DoesNotHaveReconciledTransactionsForThisMonth) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local=*/true));

  TransactionList transactions;
  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local=*/true));

  // Act & Assert
  EXPECT_FALSE(DidReconcileTransactionsThisMonth(transactions));
}

TEST_F(BraveAdsReconciledTransactionsUtilTest,
       DidReconcileTransactionsLastMonth) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local=*/true));

  TransactionList transactions;
  const TransactionInfo transaction = BuildTransactionForTesting(
      /*value=*/0.01, ConfirmationType::kViewed, /*reconciled_at=*/Now(),
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local=*/true));

  // Act & Assert
  EXPECT_TRUE(DidReconcileTransactionsLastMonth(transactions));
}

TEST_F(BraveAdsReconciledTransactionsUtilTest,
       DoesNotHaveReconciledTransactionsForPreviousMonth) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local=*/true));

  TransactionList transactions;
  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction);

  // Act & Assert
  EXPECT_FALSE(DidReconcileTransactionsLastMonth(transactions));
}

TEST_F(BraveAdsReconciledTransactionsUtilTest, DidReconcileTransaction) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local=*/true));

  const TransactionInfo transaction = BuildTransactionForTesting(
      /*value=*/0.01, ConfirmationType::kViewed, /*reconciled_at=*/Now(),
      /*should_use_random_uuids=*/true);

  // Act & Assert
  EXPECT_TRUE(DidReconcileTransaction(transaction));
}

TEST_F(BraveAdsReconciledTransactionsUtilTest, WasTransactionNotReconciled) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local=*/true));

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/true);

  // Act & Assert
  EXPECT_FALSE(DidReconcileTransaction(transaction));
}

TEST_F(BraveAdsReconciledTransactionsUtilTest,
       DidReconcileTransactionWithinDateRange) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local=*/true));

  const TransactionInfo transaction = BuildTransactionForTesting(
      /*value=*/0.01, ConfirmationType::kViewed, /*reconciled_at=*/Now(),
      /*should_use_random_uuids=*/true);

  // Act & Assert
  EXPECT_TRUE(DidReconcileTransactionWithinDateRange(transaction, DistantPast(),
                                                     Now()));
}

TEST_F(BraveAdsReconciledTransactionsUtilTest,
       HasTransactionNotReconciledForDateRange) {
  // Arrange
  AdvanceClockTo(TimeFromString("Sat, 20 Aug 2016 02:52",
                                /*is_local=*/true));  // Hello Elica!!!

  const TransactionInfo transaction = BuildUnreconciledTransactionForTesting(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/true);

  // Act & Assert
  EXPECT_FALSE(DidReconcileTransactionWithinDateRange(
      transaction, Now() + base::Milliseconds(1), DistantFuture()));
}

}  // namespace brave_ads
