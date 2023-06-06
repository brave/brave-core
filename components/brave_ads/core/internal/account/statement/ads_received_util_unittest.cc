/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/statement/ads_received_util.h"

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdsReceivedUtilTest : public UnitTestBase {};

TEST_F(BraveAdsAdsReceivedUtilTest, GetAdsReceivedForDateRange) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 =
      BuildUnreconciledTransaction(/*value*/ 0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const base::Time from_time = Now();

  const TransactionInfo transaction_2 =
      BuildUnreconciledTransaction(/*value*/ 0.0, ConfirmationType::kClicked);
  transactions.push_back(transaction_2);

  const TransactionInfo transaction_3 =
      BuildUnreconciledTransaction(/*value*/ 0.03, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  AdvanceClockTo(TimeFromString("1 January 2021", /*is_local*/ true));

  const TransactionInfo transaction_4 =
      BuildUnreconciledTransaction(/*value*/ 0.02, ConfirmationType::kViewed);
  transactions.push_back(transaction_4);

  const base::Time to_time = DistantFuture();

  // Act

  // Assert
  EXPECT_EQ(2U, GetAdsReceivedForDateRange(transactions, from_time, to_time));
}

TEST_F(BraveAdsAdsReceivedUtilTest, DoNotGetAdsReceivedForDateRange) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 =
      BuildUnreconciledTransaction(/*value*/ 0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 =
      BuildUnreconciledTransaction(/*value*/ 0.0, ConfirmationType::kClicked);
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("1 January 2021", /*is_local*/ true));

  const base::Time from_time = Now();
  const base::Time to_time = DistantFuture();

  // Act

  // Assert
  EXPECT_EQ(0U, GetAdsReceivedForDateRange(transactions, from_time, to_time));
}

TEST_F(BraveAdsAdsReceivedUtilTest, GetAdsReceivedForNoTransactions) {
  // Arrange
  const TransactionList transactions;

  const base::Time from_time = DistantPast();
  const base::Time to_time = DistantFuture();

  // Act

  // Assert
  EXPECT_EQ(0U, GetAdsReceivedForDateRange(transactions, from_time, to_time));
}

}  // namespace brave_ads
