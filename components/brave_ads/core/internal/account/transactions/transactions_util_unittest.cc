/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_util.h"

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTransactionsUtilTest : public UnitTestBase {};

TEST_F(BraveAdsTransactionsUtilTest, GetTransactionsForDateRange) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local=*/true));

  TransactionList transactions;

  const TransactionInfo transaction_1 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_1);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local=*/true));

  const TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.03, ConfirmationType::kClicked,
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_2);

  // Act
  const TransactionList transactions_for_date_range =
      GetTransactionsForDateRange(transactions, Now(), DistantFuture());

  // Assert
  EXPECT_EQ(TransactionList{transaction_2}, transactions_for_date_range);
}

TEST_F(BraveAdsTransactionsUtilTest, DoNotGetTransactionsForDateRange) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local=*/true));

  TransactionList transactions;

  const TransactionInfo transaction_1 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.03, ConfirmationType::kClicked,
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local=*/true));

  // Act
  const TransactionList transactions_for_date_range =
      GetTransactionsForDateRange(transactions, Now(), DistantFuture());

  // Assert
  EXPECT_TRUE(transactions_for_date_range.empty());
}

}  // namespace brave_ads
