/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/earnings_util.h"

#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/number_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsEarningsUtilTest : public UnitTestBase {
 protected:
  BatAdsEarningsUtilTest() = default;

  ~BatAdsEarningsUtilTest() override = default;
};

TEST_F(BatAdsEarningsUtilTest, GetEarningsForDateRange) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  const base::Time& from_time = Now();

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.03, ConfirmationType::kViewed);
  transactions.push_back(transaction_2);

  AdvanceClock(TimeFromString("1 January 2021", /* is_local */ true));

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.02, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  const base::Time& to_time = DistantFuture();

  // Act
  const double earnings =
      GetEarningsForDateRange(transactions, from_time, to_time);

  // Assert
  const double expected_earnings = 0.05;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

TEST_F(BatAdsEarningsUtilTest, DoNotGetEarningsForDateRange) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.03, ConfirmationType::kViewed);
  transactions.push_back(transaction_2);

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.02, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  AdvanceClock(TimeFromString("1 January 2021", /* is_local */ true));

  const base::Time& from_time = Now();
  const base::Time& to_time = DistantFuture();

  // Act
  const double earnings =
      GetEarningsForDateRange(transactions, from_time, to_time);

  // Assert
  const double expected_earnings = 0.0;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

TEST_F(BatAdsEarningsUtilTest, GetEarningsForNoTransactions) {
  // Arrange
  const TransactionList transactions;

  const base::Time& from_time = DistantPast();
  const base::Time& to_time = DistantFuture();

  // Act
  const double earnings =
      GetEarningsForDateRange(transactions, from_time, to_time);

  // Assert
  const double expected_earnings = 0.0;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

TEST_F(BatAdsEarningsUtilTest, GetUnreconciledEarningsForDateRange) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  const base::Time& from_time = Now();

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.03, ConfirmationType::kViewed);
  transactions.push_back(transaction_2);

  AdvanceClock(TimeFromString("1 January 2021", /* is_local */ true));

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.02, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_3);

  const base::Time& to_time = DistantFuture();

  // Act
  const double earnings =
      GetUnreconciledEarningsForDateRange(transactions, from_time, to_time);

  // Assert
  const double expected_earnings = 0.03;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

TEST_F(BatAdsEarningsUtilTest, DoNotGetUnreconciledEarningsForDateRange) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.03, ConfirmationType::kViewed);
  transactions.push_back(transaction_2);

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.02, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  AdvanceClock(TimeFromString("1 January 2021", /* is_local */ true));

  const base::Time& from_time = Now();
  const base::Time& to_time = DistantFuture();

  // Act
  const double earnings =
      GetUnreconciledEarningsForDateRange(transactions, from_time, to_time);

  // Assert
  const double expected_earnings = 0.0;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

TEST_F(BatAdsEarningsUtilTest, GetUnreconciledEarningsForNoTransactions) {
  // Arrange
  const TransactionList transactions;

  const base::Time& from_time = DistantPast();
  const base::Time& to_time = DistantFuture();

  // Act
  const double earnings =
      GetUnreconciledEarningsForDateRange(transactions, from_time, to_time);

  // Assert
  const double expected_earnings = 0.0;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

TEST_F(BatAdsEarningsUtilTest, GetReconciledEarningsForDateRange) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_2);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  const base::Time& from_time = Now();

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.03, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_3);

  AdvanceClock(TimeFromString("1 January 2021", /* is_local */ true));

  const TransactionInfo& transaction_4 =
      BuildTransaction(0.02, ConfirmationType::kViewed);
  transactions.push_back(transaction_4);

  const TransactionInfo& transaction_5 =
      BuildTransaction(0.02, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_5);

  const base::Time& to_time = DistantFuture();

  // Act
  const double earnings =
      GetReconciledEarningsForDateRange(transactions, from_time, to_time);

  // Assert
  const double expected_earnings = 0.05;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

TEST_F(BatAdsEarningsUtilTest, DoNotGetReconciledEarningsForDateRange) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.03, ConfirmationType::kViewed);
  transactions.push_back(transaction_2);

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.02, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_3);

  AdvanceClock(TimeFromString("1 January 2021", /* is_local */ true));

  const base::Time& from_time = Now();
  const base::Time& to_time = DistantFuture();

  // Act
  const double earnings =
      GetReconciledEarningsForDateRange(transactions, from_time, to_time);

  // Assert
  const double expected_earnings = 0.0;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

TEST_F(BatAdsEarningsUtilTest, GetReconciledEarningsForNoTransactions) {
  // Arrange
  const TransactionList transactions;

  const base::Time& from_time = DistantPast();
  const base::Time& to_time = DistantFuture();

  // Act
  const double earnings =
      GetReconciledEarningsForDateRange(transactions, from_time, to_time);

  // Assert
  const double expected_earnings = 0.0;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

}  // namespace ads
