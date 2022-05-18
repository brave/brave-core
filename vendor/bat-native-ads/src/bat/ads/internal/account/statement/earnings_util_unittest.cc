/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/earnings_util.h"

#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/base/number_util.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsEarningsUtilTest : public UnitTestBase {
 protected:
  BatAdsEarningsUtilTest() = default;

  ~BatAdsEarningsUtilTest() override = default;
};

TEST_F(BatAdsEarningsUtilTest, GetUnreconciledEarnings) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.04, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_2);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.03, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  const TransactionInfo& transaction_4 =
      BuildTransaction(0.05, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_4);

  AdvanceClock(TimeFromString("1 January 2021", /* is_local */ true));

  const TransactionInfo& transaction_5 =
      BuildTransaction(0.02, ConfirmationType::kViewed);
  transactions.push_back(transaction_5);

  // Act
  const double earnings = GetUnreconciledEarnings(transactions);

  // Assert
  const double expected_earnings = 0.09;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

TEST_F(BatAdsEarningsUtilTest, GetUnreconciledEarningsForNoTransactions) {
  // Arrange
  const TransactionList transactions;

  // Act
  const double earnings = GetUnreconciledEarnings(transactions);

  // Assert
  const double expected_earnings = 0.0;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

TEST_F(BatAdsEarningsUtilTest, GetReconciledEarningsForThisMonth) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.04, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_2);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.03, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  const TransactionInfo& transaction_4 =
      BuildTransaction(0.05, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_4);

  AdvanceClock(TimeFromString("1 January 2021", /* is_local */ true));

  const TransactionInfo& transaction_5 =
      BuildTransaction(0.02, ConfirmationType::kViewed);
  transactions.push_back(transaction_5);

  const TransactionInfo& transaction_6 =
      BuildTransaction(0.05, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_6);

  const TransactionInfo& transaction_7 =
      BuildTransaction(0.03, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_7);

  // Act
  const double earnings = GetReconciledEarningsForThisMonth(transactions);

  // Assert
  const double expected_earnings = 0.08;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

TEST_F(BatAdsEarningsUtilTest,
       GetReconciledEarningsForThisMonthForNoTransactions) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.04, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_2);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.03, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  const TransactionInfo& transaction_4 =
      BuildTransaction(0.05, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_4);

  AdvanceClock(TimeFromString("1 January 2021", /* is_local */ true));

  const TransactionInfo& transaction_5 =
      BuildTransaction(0.02, ConfirmationType::kViewed);
  transactions.push_back(transaction_5);

  // Act
  const double earnings = GetReconciledEarningsForThisMonth(transactions);

  // Assert
  const double expected_earnings = 0.0;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

TEST_F(BatAdsEarningsUtilTest, GetReconciledEarningsForLastMonth) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.04, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_2);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.03, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  const TransactionInfo& transaction_4 =
      BuildTransaction(0.05, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_4);

  const TransactionInfo& transaction_5 =
      BuildTransaction(0.07, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_5);

  AdvanceClock(TimeFromString("1 January 2021", /* is_local */ true));

  const TransactionInfo& transaction_6 =
      BuildTransaction(0.02, ConfirmationType::kViewed);
  transactions.push_back(transaction_6);

  const TransactionInfo& transaction_7 =
      BuildTransaction(0.05, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_7);

  const TransactionInfo& transaction_8 =
      BuildTransaction(0.03, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_8);

  // Act
  const double earnings = GetReconciledEarningsForLastMonth(transactions);

  // Assert
  const double expected_earnings = 0.12;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

TEST_F(BatAdsEarningsUtilTest,
       GetReconciledEarningsForLastMonthForNoTransactions) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.04, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_2);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.03, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  AdvanceClock(TimeFromString("1 January 2021", /* is_local */ true));

  const TransactionInfo& transaction_4 =
      BuildTransaction(0.02, ConfirmationType::kViewed);
  transactions.push_back(transaction_4);

  const TransactionInfo& transaction_5 =
      BuildTransaction(0.05, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_5);

  const TransactionInfo& transaction_6 =
      BuildTransaction(0.03, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_6);

  // Act
  const double earnings = GetReconciledEarningsForLastMonth(transactions);

  // Assert
  const double expected_earnings = 0.0;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

}  // namespace ads
