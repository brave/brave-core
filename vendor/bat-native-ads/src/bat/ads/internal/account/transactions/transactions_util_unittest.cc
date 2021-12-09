/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/transactions/transactions_util.h"

#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsTransactionsUtilTest : public UnitTestBase {
 protected:
  BatAdsTransactionsUtilTest() = default;

  ~BatAdsTransactionsUtilTest() override = default;
};

TEST_F(BatAdsTransactionsUtilTest, GetTransactionsForDateRange) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.03, ConfirmationType::kClicked);
  transactions.push_back(transaction_2);

  const base::Time& from_time = Now();
  const base::Time& to_time = DistantFuture();

  // Act
  const TransactionList& transactions_for_date_range =
      GetTransactionsForDateRange(transactions, from_time, to_time);

  // Assert
  TransactionList expected_transactions_for_date_range;
  expected_transactions_for_date_range.push_back(transaction_2);

  EXPECT_EQ(expected_transactions_for_date_range, transactions_for_date_range);
}

TEST_F(BatAdsTransactionsUtilTest, DoNotGetTransactionsForDateRange) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.03, ConfirmationType::kClicked);
  transactions.push_back(transaction_2);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  const base::Time& from_time = Now();
  const base::Time& to_time = DistantFuture();

  // Act
  const TransactionList& transactions_for_date_range =
      GetTransactionsForDateRange(transactions, from_time, to_time);

  // Assert
  const TransactionList expected_transactions_for_date_range;

  EXPECT_EQ(expected_transactions_for_date_range, transactions_for_date_range);
}

}  // namespace ads
