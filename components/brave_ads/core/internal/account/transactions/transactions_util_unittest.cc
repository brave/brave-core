/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_util.h"

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsTransactionsUtilTest : public UnitTestBase {};

TEST_F(BatAdsTransactionsUtilTest, GetTransactionsForDateRange) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_2 =
      BuildTransaction(/*value*/ 0.03, ConfirmationType::kClicked);
  transactions.push_back(transaction_2);

  const base::Time from_time = Now();
  const base::Time to_time = DistantFuture();

  // Act
  const TransactionList transactions_for_date_range =
      GetTransactionsForDateRange(transactions, from_time, to_time);

  // Assert
  const TransactionList expected_transactions_for_date_range = {transaction_2};
  EXPECT_EQ(expected_transactions_for_date_range, transactions_for_date_range);
}

TEST_F(BatAdsTransactionsUtilTest, DoNotGetTransactionsForDateRange) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 =
      BuildTransaction(/*value*/ 0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 =
      BuildTransaction(/*value*/ 0.03, ConfirmationType::kClicked);
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const base::Time from_time = Now();
  const base::Time to_time = DistantFuture();

  // Act
  const TransactionList transactions_for_date_range =
      GetTransactionsForDateRange(transactions, from_time, to_time);

  // Assert
  EXPECT_TRUE(transactions_for_date_range.empty());
}

}  // namespace brave_ads
