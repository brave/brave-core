/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/statement.h"

#include "base/functional/bind.h"
#include "bat/ads/internal/account/transactions/transaction_info.h"
#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsStatementTest : public UnitTestBase {};

TEST_F(BatAdsStatementTest, GetForTransactionsThisMonth) {
  // Arrange
  AdvanceClockTo(TimeFromString("18 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_2);

  SaveTransactions(transactions);

  // Act
  BuildStatement(base::BindOnce([](mojom::StatementInfoPtr statement) {
    ASSERT_TRUE(statement);

    mojom::StatementInfoPtr expected_statement = mojom::StatementInfo::New();
    expected_statement->earnings_last_month = 0.0;
    expected_statement->earnings_this_month = 0.02;
    expected_statement->next_payment_date =
        TimeFromString("7 December 2020 23:59:59.999", /*is_local*/ false);
    expected_statement->ads_received_this_month = 2;

    EXPECT_EQ(expected_statement, statement);
  }));

  // Assert
}

TEST_F(BatAdsStatementTest, GetForTransactionsSplitOverThreeConsecutiveMonths) {
  // Arrange
  TransactionList transactions;

  AdvanceClockTo(TimeFromString("31 October 2020", /*is_local*/ true));

  const TransactionInfo transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("18 November 2020", /*is_local*/ true));

  const TransactionInfo transaction_3 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_4);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_5 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_5);

  const TransactionInfo transaction_6 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_6);

  const TransactionInfo transaction_7 =
      BuildTransaction(0.0, ConfirmationType::kClicked);
  transactions.push_back(transaction_7);

  const TransactionInfo transaction_8 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_8);

  SaveTransactions(transactions);

  // Act
  BuildStatement(base::BindOnce([](mojom::StatementInfoPtr statement) {
    ASSERT_TRUE(statement);

    mojom::StatementInfoPtr expected_statement = mojom::StatementInfo::New();
    expected_statement->earnings_last_month = 0.01;
    expected_statement->earnings_this_month = 0.05;
    expected_statement->next_payment_date =
        TimeFromString("7 January 2021 23:59:59.999", /*is_local*/ false);
    expected_statement->ads_received_this_month = 3;

    EXPECT_EQ(expected_statement, statement);
  }));

  // Assert
}

TEST_F(BatAdsStatementTest, GetForTransactionsSplitOverTwoYears) {
  // Arrange
  TransactionList transactions;

  AdvanceClockTo(TimeFromString("31 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("1 January 2021", /*is_local*/ true));

  const TransactionInfo transaction_3 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_4);

  const TransactionInfo transaction_5 =
      BuildTransaction(0.0, ConfirmationType::kClicked);
  transactions.push_back(transaction_5);

  const TransactionInfo transaction_6 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_6);

  SaveTransactions(transactions);

  // Act
  BuildStatement(base::BindOnce([](mojom::StatementInfoPtr statement) {
    ASSERT_TRUE(statement);

    mojom::StatementInfoPtr expected_statement = mojom::StatementInfo::New();
    expected_statement->earnings_last_month = 0.01;
    expected_statement->earnings_this_month = 0.04;
    expected_statement->next_payment_date =
        TimeFromString("7 January 2021 23:59:59.999", /*is_local*/ false);
    expected_statement->ads_received_this_month = 3;

    EXPECT_EQ(expected_statement, statement);
  }));

  // Assert
}

TEST_F(BatAdsStatementTest, GetForNoTransactions) {
  // Arrange
  AdvanceClockTo(TimeFromString("18 November 2020", /*is_local*/ true));

  // Act
  BuildStatement(base::BindOnce([](mojom::StatementInfoPtr statement) {
    ASSERT_TRUE(statement);

    mojom::StatementInfoPtr expected_statement = mojom::StatementInfo::New();
    expected_statement->earnings_last_month = 0.0;
    expected_statement->earnings_this_month = 0.0;
    expected_statement->next_payment_date =
        TimeFromString("7 January 2021 23:59:59.999", /*is_local*/ false);
    expected_statement->ads_received_this_month = 0;

    EXPECT_EQ(expected_statement, statement);
  }));

  // Assert
}

}  // namespace ads
