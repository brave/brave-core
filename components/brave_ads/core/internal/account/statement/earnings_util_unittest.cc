/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/statement/earnings_util.h"

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsEarningsUtilTest : public UnitTestBase {};

TEST_F(BraveAdsEarningsUtilTest, GetUnreconciledEarnings) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 =
      BuildUnreconciledTransaction(/*value*/ 0.04, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_3 =
      BuildUnreconciledTransaction(/*value*/ 0.03, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 = BuildTransaction(
      /*value*/ 0.05, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_4);

  AdvanceClockTo(TimeFromString("1 January 2021", /*is_local*/ true));

  const TransactionInfo transaction_5 =
      BuildUnreconciledTransaction(/*value*/ 0.02, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_5);

  // Act
  const double earnings = GetUnreconciledEarnings(transactions);

  // Assert
  EXPECT_DOUBLE_EQ(0.09, earnings);
}

TEST_F(BraveAdsEarningsUtilTest, GetUnreconciledEarningsForNoTransactions) {
  // Arrange
  const TransactionList transactions;

  // Act
  const double earnings = GetUnreconciledEarnings(transactions);

  // Assert
  EXPECT_DOUBLE_EQ(0.0, earnings);
}

TEST_F(BraveAdsEarningsUtilTest, GetReconciledEarningsForThisMonth) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 =
      BuildUnreconciledTransaction(/*value*/ 0.04, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_3 =
      BuildUnreconciledTransaction(/*value*/ 0.03, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 = BuildTransaction(
      /*value*/ 0.05, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_4);

  AdvanceClockTo(TimeFromString("1 January 2021", /*is_local*/ true));

  const TransactionInfo transaction_5 =
      BuildUnreconciledTransaction(/*value*/ 0.02, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_5);

  const TransactionInfo transaction_6 = BuildTransaction(
      /*value*/ 0.05, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_6);

  const TransactionInfo transaction_7 = BuildTransaction(
      /*value*/ 0.03, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_7);

  // Act
  const double earnings = GetReconciledEarningsForThisMonth(transactions);

  // Assert
  EXPECT_DOUBLE_EQ(0.08, earnings);
}

TEST_F(BraveAdsEarningsUtilTest,
       GetReconciledEarningsForThisMonthForNoTransactions) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 =
      BuildUnreconciledTransaction(/*value*/ 0.04, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_3 =
      BuildUnreconciledTransaction(/*value*/ 0.03, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 = BuildTransaction(
      /*value*/ 0.05, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_4);

  AdvanceClockTo(TimeFromString("1 January 2021", /*is_local*/ true));

  const TransactionInfo transaction_5 =
      BuildUnreconciledTransaction(/*value*/ 0.02, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_5);

  // Act
  const double earnings = GetReconciledEarningsForThisMonth(transactions);

  // Assert
  EXPECT_DOUBLE_EQ(0.0, earnings);
}

TEST_F(BraveAdsEarningsUtilTest, GetReconciledEarningsForLastMonth) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 =
      BuildUnreconciledTransaction(/*value*/ 0.04, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_3 =
      BuildUnreconciledTransaction(/*value*/ 0.03, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 = BuildTransaction(
      /*value*/ 0.05, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_4);

  const TransactionInfo transaction_5 = BuildTransaction(
      /*value*/ 0.07, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_5);

  AdvanceClockTo(TimeFromString("1 January 2021", /*is_local*/ true));

  const TransactionInfo transaction_6 =
      BuildUnreconciledTransaction(/*value*/ 0.02, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_6);

  const TransactionInfo transaction_7 = BuildTransaction(
      /*value*/ 0.05, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_7);

  const TransactionInfo transaction_8 = BuildTransaction(
      /*value*/ 0.03, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_8);

  // Act
  const double earnings = GetReconciledEarningsForLastMonth(transactions);

  // Assert
  EXPECT_DOUBLE_EQ(0.12, earnings);
}

TEST_F(BraveAdsEarningsUtilTest,
       GetReconciledEarningsForLastMonthForNoTransactions) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 =
      BuildUnreconciledTransaction(/*value*/ 0.04, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_3 =
      BuildUnreconciledTransaction(/*value*/ 0.03, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_3);

  AdvanceClockTo(TimeFromString("1 January 2021", /*is_local*/ true));

  const TransactionInfo transaction_4 =
      BuildUnreconciledTransaction(/*value*/ 0.02, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_4);

  const TransactionInfo transaction_5 = BuildTransaction(
      /*value*/ 0.05, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_5);

  const TransactionInfo transaction_6 = BuildTransaction(
      /*value*/ 0.03, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_6);

  // Act
  const double earnings = GetReconciledEarningsForLastMonth(transactions);

  // Assert
  EXPECT_DOUBLE_EQ(0.0, earnings);
}

}  // namespace brave_ads
