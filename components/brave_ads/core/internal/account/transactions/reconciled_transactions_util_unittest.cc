/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/reconciled_transactions_util.h"

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsReconciledTransactionsUtilTest : public UnitTestBase {};

TEST_F(BatAdsReconciledTransactionsUtilTest,
       DidReconcileTransactionsThisMonth) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;
  const TransactionInfo transaction =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction);

  // Act

  // Assert
  EXPECT_TRUE(DidReconcileTransactionsThisMonth(transactions));
}

TEST_F(BatAdsReconciledTransactionsUtilTest,
       DoesNotHaveReconciledTransactionsForThisMonth) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;
  const TransactionInfo transaction =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  // Act

  // Assert
  EXPECT_FALSE(DidReconcileTransactionsThisMonth(transactions));
}

TEST_F(BatAdsReconciledTransactionsUtilTest,
       DidReconcileTransactionsLastMonth) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;
  const TransactionInfo transaction =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  // Act

  // Assert
  EXPECT_TRUE(DidReconcileTransactionsLastMonth(transactions));
}

TEST_F(BatAdsReconciledTransactionsUtilTest,
       DoesNotHaveReconciledTransactionsForPreviousMonth) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;
  const TransactionInfo transaction =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction);

  // Act

  // Assert
  EXPECT_FALSE(DidReconcileTransactionsLastMonth(transactions));
}

TEST_F(BatAdsReconciledTransactionsUtilTest, DidReconcileTransaction) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  const TransactionInfo transaction =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed, Now());

  // Act

  // Assert
  EXPECT_TRUE(DidReconcileTransaction(transaction));
}

TEST_F(BatAdsReconciledTransactionsUtilTest, WasTransactionNotReconciled) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  const TransactionInfo transaction =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed);

  // Act

  // Assert
  EXPECT_FALSE(DidReconcileTransaction(transaction));
}

TEST_F(BatAdsReconciledTransactionsUtilTest,
       DidReconcileTransactionWithinDateRange) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  const TransactionInfo transaction =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed, Now());

  // Act

  // Assert
  EXPECT_TRUE(DidReconcileTransactionWithinDateRange(transaction, DistantPast(),
                                                     Now()));
}

TEST_F(BatAdsReconciledTransactionsUtilTest,
       HasTransactionNotReconciledForDateRange) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  const TransactionInfo transaction =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed);

  // Act

  // Assert
  EXPECT_FALSE(DidReconcileTransactionWithinDateRange(
      transaction, Now() + base::Milliseconds(1), DistantFuture()));
}

}  // namespace brave_ads
