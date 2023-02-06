/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/ads_received_util.h"

#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsAdsReceivedUtilTest : public UnitTestBase {};

TEST_F(BatAdsAdsReceivedUtilTest, GetAdsReceivedForDateRange) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const base::Time from_time = Now();

  const TransactionInfo transaction_2 =
      BuildTransaction(0.0, ConfirmationType::kClicked);
  transactions.push_back(transaction_2);

  const TransactionInfo transaction_3 =
      BuildTransaction(0.03, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  AdvanceClockTo(TimeFromString("1 January 2021", /*is_local*/ true));

  const TransactionInfo transaction_4 =
      BuildTransaction(0.02, ConfirmationType::kViewed);
  transactions.push_back(transaction_4);

  const base::Time to_time = DistantFuture();

  // Act
  const int ads_received =
      GetAdsReceivedForDateRange(transactions, from_time, to_time);

  // Assert
  constexpr int kExpectedAdsReceived = 2;
  EXPECT_EQ(kExpectedAdsReceived, ads_received);
}

TEST_F(BatAdsAdsReceivedUtilTest, DoNotGetAdsReceivedForDateRange) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 =
      BuildTransaction(0.0, ConfirmationType::kClicked);
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("1 January 2021", /*is_local*/ true));

  const base::Time from_time = Now();
  const base::Time to_time = DistantFuture();

  // Act
  const int ads_received =
      GetAdsReceivedForDateRange(transactions, from_time, to_time);

  // Assert
  constexpr int kExpectedAdsReceived = 0;
  EXPECT_EQ(kExpectedAdsReceived, ads_received);
}

TEST_F(BatAdsAdsReceivedUtilTest, GetAdsReceivedForNoTransactions) {
  // Arrange
  const TransactionList transactions;

  const base::Time from_time = DistantPast();
  const base::Time to_time = DistantFuture();

  // Act
  const int ads_received =
      GetAdsReceivedForDateRange(transactions, from_time, to_time);

  // Assert
  constexpr int kExpectedAdsReceived = 0;
  EXPECT_EQ(kExpectedAdsReceived, ads_received);
}

}  // namespace ads
