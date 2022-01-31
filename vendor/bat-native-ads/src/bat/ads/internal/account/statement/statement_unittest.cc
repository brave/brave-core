/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/statement.h"

#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/statement_info.h"
#include "bat/ads/transaction_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

using ::testing::NiceMock;

namespace ads {

class BatAdsStatementTest : public UnitTestBase {
 protected:
  BatAdsStatementTest() = default;

  ~BatAdsStatementTest() override = default;
};

TEST_F(BatAdsStatementTest, GetForTransactionsThisMonth) {
  // Arrange
  AdvanceClock(TimeFromString("18 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_2);

  SaveTransactions(transactions);

  // Act
  BuildStatement([](const bool success, const StatementInfo& statement) {
    ASSERT_TRUE(success);

    StatementInfo expected_statement;
    expected_statement.next_payment_date = TimestampFromString(
        "5 December 2020 23:59:59.999", /* is_local */ false);
    expected_statement.earnings_this_month = 0.02;
    expected_statement.earnings_last_month = 0.0;
    expected_statement.ads_received_this_month = 2;

    EXPECT_EQ(expected_statement, statement);
  });

  // Assert
}

TEST_F(BatAdsStatementTest, GetForTransactionsSplitOverThreeConsecutiveMonths) {
  // Arrange
  TransactionList transactions;

  AdvanceClock(TimeFromString("31 October 2020", /* is_local */ true));

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_2);

  AdvanceClock(TimeFromString("18 November 2020", /* is_local */ true));

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  const TransactionInfo& transaction_4 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_4);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  const TransactionInfo& transaction_5 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_5);

  const TransactionInfo& transaction_6 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_6);

  const TransactionInfo& transaction_7 =
      BuildTransaction(0.0, ConfirmationType::kClicked);
  transactions.push_back(transaction_7);

  const TransactionInfo& transaction_8 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_8);

  SaveTransactions(transactions);

  // Act
  BuildStatement([](const bool success, const StatementInfo& statement) {
    ASSERT_TRUE(success);

    StatementInfo expected_statement;
    expected_statement.next_payment_date = TimestampFromString(
        "5 January 2021 23:59:59.999", /* is_local */ false);
    expected_statement.earnings_this_month = 0.05;
    expected_statement.earnings_last_month = 0.01;
    expected_statement.ads_received_this_month = 3;

    EXPECT_EQ(expected_statement, statement);
  });

  // Assert
}

TEST_F(BatAdsStatementTest, GetForTransactionsSplitOverTwoYears) {
  // Arrange
  TransactionList transactions;

  AdvanceClock(TimeFromString("31 December 2020", /* is_local */ true));

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_2);

  AdvanceClock(TimeFromString("1 January 2021", /* is_local */ true));

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  const TransactionInfo& transaction_4 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_4);

  const TransactionInfo& transaction_5 =
      BuildTransaction(0.0, ConfirmationType::kClicked);
  transactions.push_back(transaction_5);

  const TransactionInfo& transaction_6 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_6);

  SaveTransactions(transactions);

  // Act
  BuildStatement([](const bool success, const StatementInfo& statement) {
    ASSERT_TRUE(success);

    StatementInfo expected_statement;
    expected_statement.next_payment_date = TimestampFromString(
        "5 January 2021 23:59:59.999", /* is_local */ false);
    expected_statement.earnings_this_month = 0.04;
    expected_statement.earnings_last_month = 0.01;
    expected_statement.ads_received_this_month = 3;

    EXPECT_EQ(expected_statement, statement);
  });

  // Assert
}

TEST_F(BatAdsStatementTest, GetForNoTransactions) {
  // Arrange
  AdvanceClock(TimeFromString("18 November 2020", /* is_local */ true));

  // Act
  BuildStatement([](const bool success, const StatementInfo& statement) {
    ASSERT_TRUE(success);

    StatementInfo expected_statement;
    expected_statement.next_payment_date = TimestampFromString(
        "5 January 2021 23:59:59.999", /* is_local */ false);
    expected_statement.earnings_this_month = 0.0;
    expected_statement.earnings_last_month = 0.0;
    expected_statement.ads_received_this_month = 0;

    EXPECT_EQ(expected_statement, statement);
  });

  // Assert
}

}  // namespace ads
