/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/transactions/reconciled_transactions_util.h"

#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsReconciledTransactionsUtilTest : public UnitTestBase {
 protected:
  BatAdsReconciledTransactionsUtilTest() = default;

  ~BatAdsReconciledTransactionsUtilTest() override = default;
};

TEST_F(BatAdsReconciledTransactionsUtilTest,
       DidReconcileTransactionsThisMonth) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;
  const TransactionInfo& transaction =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction);

  // Act
  const bool did_reconcile_transactions =
      DidReconcileTransactionsThisMonth(transactions);

  // Assert
  EXPECT_TRUE(did_reconcile_transactions);
}

TEST_F(BatAdsReconciledTransactionsUtilTest,
       DoesNotHaveReconciledTransactionsForThisMonth) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;
  const TransactionInfo& transaction =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  // Act
  const bool did_reconcile_transactions =
      DidReconcileTransactionsThisMonth(transactions);

  // Assert
  EXPECT_FALSE(did_reconcile_transactions);
}

TEST_F(BatAdsReconciledTransactionsUtilTest,
       DidReconcileTransactionsLastMonth) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;
  const TransactionInfo& transaction =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  // Act
  const bool did_reconcile_transactions =
      DidReconcileTransactionsLastMonth(transactions);

  // Assert
  EXPECT_TRUE(did_reconcile_transactions);
}

TEST_F(BatAdsReconciledTransactionsUtilTest,
       DoesNotHaveReconciledTransactionsForPreviousMonth) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;
  const TransactionInfo& transaction =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction);

  // Act
  const bool did_reconcile_transactions =
      DidReconcileTransactionsLastMonth(transactions);

  // Assert
  EXPECT_FALSE(did_reconcile_transactions);
}

TEST_F(BatAdsReconciledTransactionsUtilTest, DidReconcileTransaction) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  const TransactionInfo& transaction =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());

  // Act
  const bool did_reconcile_transaction = DidReconcileTransaction(transaction);

  // Assert
  EXPECT_TRUE(did_reconcile_transaction);
}

TEST_F(BatAdsReconciledTransactionsUtilTest, WasTransactionNotReconciled) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  const TransactionInfo& transaction =
      BuildTransaction(0.01, ConfirmationType::kViewed);

  // Act
  const bool did_reconcile_transaction = DidReconcileTransaction(transaction);

  // Assert
  EXPECT_FALSE(did_reconcile_transaction);
}

TEST_F(BatAdsReconciledTransactionsUtilTest,
       DidReconcileTransactionWithinDateRange) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  const TransactionInfo& transaction =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());

  // Act
  const bool did_reconcile_transaction =
      DidReconcileTransactionWithinDateRange(transaction, DistantPast(), Now());

  // Assert
  EXPECT_TRUE(did_reconcile_transaction);
}

TEST_F(BatAdsReconciledTransactionsUtilTest,
       HasTransactionNotReconciledForDateRange) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  const TransactionInfo& transaction =
      BuildTransaction(0.01, ConfirmationType::kViewed);

  // Act
  const bool did_reconcile_transaction = DidReconcileTransactionWithinDateRange(
      transaction, Now() + base::Seconds(1), DistantFuture());

  // Assert
  EXPECT_FALSE(did_reconcile_transaction);
}

}  // namespace ads
