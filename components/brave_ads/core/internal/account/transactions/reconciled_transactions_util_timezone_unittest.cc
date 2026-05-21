/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string_view>

#include "brave/components/brave_ads/core/internal/account/transactions/reconciled_transactions_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/test/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/common/test/scoped_timezone_for_testing.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/timezone_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

// Parameterized by timezone to verify that month-boundary reconciliation checks
// use local time correctly regardless of DST, hemisphere, or UTC offset. All
// test timestamps are local time via TimeFromString.

class BraveAdsReconciledTransactionsUtilTimezoneTest
    : public test::TestBase,
      public ::testing::WithParamInterface<std::string_view> {
 protected:
  const test::ScopedTimezoneForTesting scoped_timezone_{GetParam()};
};

INSTANTIATE_TEST_SUITE_P(BraveAdsTimezones,
                         BraveAdsReconciledTransactionsUtilTimezoneTest,
                         test::kTimezones,
                         test::TimezoneTestParamName);

TEST_P(BraveAdsReconciledTransactionsUtilTimezoneTest,
       DidReconcileTransactionsThisMonth) {
  // Arrange
  AdvanceClockTo(
      test::TimeFromString("Wed, 16 Sep 2015 23:01"));  // Hello Millie!!!

  TransactionList transactions;
  const TransactionInfo transaction = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*use_random_uuids=*/true);
  transactions.push_back(transaction);

  // Act & Assert
  EXPECT_TRUE(DidReconcileTransactionsThisMonth(transactions));
}

TEST_P(BraveAdsReconciledTransactionsUtilTimezoneTest,
       DidNotReconcileTransactionsThisMonth) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("5 November 2020"));

  TransactionList transactions;
  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*use_random_uuids=*/true);
  transactions.push_back(transaction);

  AdvanceClockTo(test::TimeFromString("25 December 2020"));

  // Act & Assert
  EXPECT_FALSE(DidReconcileTransactionsThisMonth(transactions));
}

TEST_P(BraveAdsReconciledTransactionsUtilTimezoneTest,
       DidReconcileTransactionsPreviousMonth) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("5 November 2020"));

  TransactionList transactions;
  const TransactionInfo transaction = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*use_random_uuids=*/true);
  transactions.push_back(transaction);

  AdvanceClockTo(test::TimeFromString("25 December 2020"));

  // Act & Assert
  EXPECT_TRUE(DidReconcileTransactionsPreviousMonth(transactions));
}

TEST_P(BraveAdsReconciledTransactionsUtilTimezoneTest,
       DidNotReconcileTransactionsPreviousMonth) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("5 November 2020"));

  TransactionList transactions;
  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*use_random_uuids=*/true);
  transactions.push_back(transaction);

  // Act & Assert
  EXPECT_FALSE(DidReconcileTransactionsPreviousMonth(transactions));
}

}  // namespace brave_ads
