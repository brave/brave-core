/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/reconciled_transactions_util.h"

#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsReconciledTransactionsUtilTest : public test::TestBase {};

TEST_F(BraveAdsReconciledTransactionsUtilTest,
       DidReconcileTransactionsThisMonth) {
  // Arrange
  AdvanceClockTo(
      test::TimeFromString("Wed, 16 Sep 2015 23:01"));  // Hello Millie!!!

  TransactionList transactions;
  const TransactionInfo transaction = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction);

  // Act & Assert
  EXPECT_TRUE(DidReconcileTransactionsThisMonth(transactions));
}

TEST_F(BraveAdsReconciledTransactionsUtilTest,
       DoesNotHaveReconciledTransactionsForThisMonth) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("5 November 2020"));

  TransactionList transactions;
  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction);

  AdvanceClockTo(test::TimeFromString("25 December 2020"));

  // Act & Assert
  EXPECT_FALSE(DidReconcileTransactionsThisMonth(transactions));
}

TEST_F(BraveAdsReconciledTransactionsUtilTest,
       DidReconcileTransactionsPreviousMonth) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("5 November 2020"));

  TransactionList transactions;
  const TransactionInfo transaction = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction);

  AdvanceClockTo(test::TimeFromString("25 December 2020"));

  // Act & Assert
  EXPECT_TRUE(DidReconcileTransactionsPreviousMonth(transactions));
}

TEST_F(BraveAdsReconciledTransactionsUtilTest,
       DoesNotHaveReconciledTransactionsForPreviousMonth) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("5 November 2020"));

  TransactionList transactions;
  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction);

  // Act & Assert
  EXPECT_FALSE(DidReconcileTransactionsPreviousMonth(transactions));
}

TEST_F(BraveAdsReconciledTransactionsUtilTest, DidReconcileTransaction) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("5 November 2020"));

  const TransactionInfo transaction = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);

  // Act & Assert
  EXPECT_TRUE(DidReconcileTransaction(transaction));
}

TEST_F(BraveAdsReconciledTransactionsUtilTest, WasTransactionNotReconciled) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("5 November 2020"));

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);

  // Act & Assert
  EXPECT_FALSE(DidReconcileTransaction(transaction));
}

TEST_F(BraveAdsReconciledTransactionsUtilTest,
       DidReconcileTransactionWithinDateRange) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("5 November 2020"));

  const TransactionInfo transaction = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);

  // Act & Assert
  EXPECT_TRUE(DidReconcileTransactionWithinDateRange(
      transaction, /*from_time=*/test::DistantPast(), /*to_time=*/test::Now()));
}

TEST_F(BraveAdsReconciledTransactionsUtilTest,
       HasTransactionNotReconciledForDateRange) {
  // Arrange
  AdvanceClockTo(
      test::TimeFromString("Sat, 20 Aug 2016 02:52"));  // Hello Elica!!!

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);

  // Act & Assert
  EXPECT_FALSE(DidReconcileTransactionWithinDateRange(
      transaction, /*from_time=*/test::Now() + base::Milliseconds(1),
      /*to_time=*/test::DistantFuture()));
}

}  // namespace brave_ads
