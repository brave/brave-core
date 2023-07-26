/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/statement/statement.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/statement/statement_feature.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsStatementTest : public UnitTestBase {};

TEST_F(BraveAdsStatementTest, GetForTransactionsThisMonth) {
  // Arrange
  AdvanceClockTo(TimeFromString("18 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 =
      BuildUnreconciledTransaction(/*value*/ 0.01, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  SaveTransactions(transactions);

  // Act
  BuildStatement(base::BindOnce([](mojom::StatementInfoPtr statement) {
    ASSERT_TRUE(statement);

    mojom::StatementInfoPtr expected_statement = mojom::StatementInfo::New();
    expected_statement->min_earnings_last_month = 0.0;
    expected_statement->max_earnings_last_month = 0.0;
    expected_statement->min_earnings_this_month =
        0.02 * kMinEstimatedEarningsMultiplier.Get();
    expected_statement->max_earnings_this_month = 0.02;
    expected_statement->next_payment_date =
        TimeFromString("7 December 2020 23:59:59.999", /*is_local*/ false);
    expected_statement->ads_received_this_month = 2;
    expected_statement->ad_types_received_this_month = {{"ad_notification", 2}};

    EXPECT_EQ(expected_statement, statement);
  }));

  // Assert
}

TEST_F(BraveAdsStatementTest,
       GetForTransactionsSplitOverThreeConsecutiveMonths) {
  // Arrange
  TransactionList transactions;

  AdvanceClockTo(TimeFromString("31 October 2020", /*is_local*/ true));

  const TransactionInfo transaction_1 =
      BuildUnreconciledTransaction(/*value*/ 0.01, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("18 November 2020", /*is_local*/ true));

  const TransactionInfo transaction_3 =
      BuildUnreconciledTransaction(/*value*/ 0.01, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_4);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_5 =
      BuildUnreconciledTransaction(/*value*/ 0.01, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_5);

  const TransactionInfo transaction_6 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_6);

  const TransactionInfo transaction_7 =
      BuildUnreconciledTransaction(/*value*/ 0.0, ConfirmationType::kClicked,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_7);

  const TransactionInfo transaction_8 =
      BuildUnreconciledTransaction(/*value*/ 0.01, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_8);

  SaveTransactions(transactions);

  // Act
  BuildStatement(base::BindOnce([](mojom::StatementInfoPtr statement) {
    ASSERT_TRUE(statement);

    mojom::StatementInfoPtr expected_statement = mojom::StatementInfo::New();
    expected_statement->min_earnings_last_month =
        0.01 * kMinEstimatedEarningsMultiplier.Get();
    expected_statement->max_earnings_last_month = 0.01;
    expected_statement->min_earnings_this_month =
        0.05 * kMinEstimatedEarningsMultiplier.Get();
    expected_statement->max_earnings_this_month = 0.05;
    expected_statement->next_payment_date =
        TimeFromString("7 January 2021 23:59:59.999", /*is_local*/ false);
    expected_statement->ads_received_this_month = 3;
    expected_statement->ad_types_received_this_month = {{"ad_notification", 3}};

    EXPECT_EQ(expected_statement, statement);
  }));

  // Assert
}

TEST_F(BraveAdsStatementTest, GetForTransactionsSplitOverTwoYears) {
  // Arrange
  TransactionList transactions;

  AdvanceClockTo(TimeFromString("31 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_1 =
      BuildUnreconciledTransaction(/*value*/ 0.01, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("1 January 2021", /*is_local*/ true));

  const TransactionInfo transaction_3 =
      BuildUnreconciledTransaction(/*value*/ 0.01, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_4);

  const TransactionInfo transaction_5 =
      BuildUnreconciledTransaction(/*value*/ 0.0, ConfirmationType::kClicked,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_5);

  const TransactionInfo transaction_6 =
      BuildUnreconciledTransaction(/*value*/ 0.01, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_6);

  SaveTransactions(transactions);

  // Act
  BuildStatement(base::BindOnce([](mojom::StatementInfoPtr statement) {
    ASSERT_TRUE(statement);

    mojom::StatementInfoPtr expected_statement = mojom::StatementInfo::New();
    expected_statement->min_earnings_last_month =
        0.01 * kMinEstimatedEarningsMultiplier.Get();
    expected_statement->max_earnings_last_month = 0.01;
    expected_statement->min_earnings_this_month =
        0.04 * kMinEstimatedEarningsMultiplier.Get();
    expected_statement->max_earnings_this_month = 0.04;
    expected_statement->next_payment_date =
        TimeFromString("7 January 2021 23:59:59.999", /*is_local*/ false);
    expected_statement->ads_received_this_month = 3;
    expected_statement->ad_types_received_this_month = {{"ad_notification", 3}};

    EXPECT_EQ(expected_statement, statement);
  }));

  // Assert
}

TEST_F(BraveAdsStatementTest, GetForNoTransactions) {
  // Arrange
  AdvanceClockTo(TimeFromString("18 November 2020", /*is_local*/ true));

  // Act
  BuildStatement(base::BindOnce([](mojom::StatementInfoPtr statement) {
    ASSERT_TRUE(statement);

    mojom::StatementInfoPtr expected_statement = mojom::StatementInfo::New();
    expected_statement->min_earnings_last_month = 0.0;
    expected_statement->max_earnings_last_month = 0.0;
    expected_statement->min_earnings_this_month = 0.0;
    expected_statement->max_earnings_this_month = 0.0;
    expected_statement->next_payment_date =
        TimeFromString("7 January 2021 23:59:59.999", /*is_local*/ false);
    expected_statement->ads_received_this_month = 0;
    expected_statement->ad_types_received_this_month = {};

    EXPECT_EQ(expected_statement, statement);
  }));

  // Assert
}

TEST_F(BraveAdsStatementTest, GetWithFilteredTransactions) {
  // Arrange
  AdvanceClockTo(TimeFromString("12 October 2020", /*is_local*/ true));
  TransactionList transactions;

  const TransactionInfo transaction_1 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  TransactionInfo transaction_2 = BuildTransaction(
      /*value*/ 0.02, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transaction_2.ad_type = AdType::kNewTabPageAd;
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("18 November 2020", /*is_local*/ true));

  const TransactionInfo transaction_3 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_3);

  TransactionInfo transaction_4 =
      BuildUnreconciledTransaction(/*value*/ 0.01, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transaction_4.ad_type = AdType::kNewTabPageAd;
  transactions.push_back(transaction_4);

  SaveTransactions(transactions);

  // Act
  BuildStatement(base::BindOnce([](mojom::StatementInfoPtr statement) {
    ASSERT_TRUE(statement);

    mojom::StatementInfoPtr expected_statement = mojom::StatementInfo::New();
    expected_statement->min_earnings_last_month =
        0.01 * kMinEstimatedEarningsMultiplier.Get();
    expected_statement->max_earnings_last_month = 0.03;
    expected_statement->min_earnings_this_month =
        0.01 * kMinEstimatedEarningsMultiplier.Get();
    expected_statement->max_earnings_this_month = 0.02;
    expected_statement->next_payment_date =
        TimeFromString("7 December 2020 23:59:59.999", /*is_local*/ false);
    expected_statement->ads_received_this_month = 2;
    expected_statement->ad_types_received_this_month = {{"ad_notification", 1},
                                                        {"new_tab_page_ad", 1}};

    EXPECT_EQ(expected_statement, statement);
  }));

  // Assert
}

}  // namespace brave_ads
