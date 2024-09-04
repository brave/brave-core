/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/statement/statement.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/account/statement/statement_feature.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsStatementTest : public test::TestBase {};

TEST_F(BraveAdsStatementTest, GetForTransactionsThisMonth) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("18 November 2020"));

  TransactionList transactions;

  const TransactionInfo transaction_1 = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_2);

  database::SaveTransactions(transactions);

  // Act & Assert
  const mojom::StatementInfoPtr expected_mojom_statement =
      mojom::StatementInfo::New();
  expected_mojom_statement->min_earnings_last_month = 0.0;
  expected_mojom_statement->max_earnings_last_month = 0.0;
  expected_mojom_statement->min_earnings_this_month =
      0.02 * kMinEstimatedEarningsMultiplier.Get();
  expected_mojom_statement->max_earnings_this_month = 0.02;
  expected_mojom_statement->next_payment_date =
      test::TimeFromUTCString("7 December 2020 23:59:59.999");
  expected_mojom_statement->ads_received_this_month = 2;
  expected_mojom_statement->ads_summary_this_month = {
      {mojom::AdType::kNotificationAd, 2}};

  base::MockCallback<BuildStatementCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_mojom_statement))));
  BuildStatement(callback.Get());
}

TEST_F(BraveAdsStatementTest,
       GetForTransactionsSplitOverThreeConsecutiveMonths) {
  // Arrange
  TransactionList transactions;

  AdvanceClockTo(test::TimeFromString("31 October 2020"));

  const TransactionInfo transaction_1 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_2);

  AdvanceClockTo(test::TimeFromString("18 November 2020"));

  const TransactionInfo transaction_3 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_4);

  AdvanceClockTo(test::TimeFromString("25 December 2020"));

  const TransactionInfo transaction_5 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_5);

  const TransactionInfo transaction_6 = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_6);

  const TransactionInfo transaction_7 = test::BuildUnreconciledTransaction(
      /*value=*/0.0, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kClicked,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_7);

  const TransactionInfo transaction_8 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_8);

  database::SaveTransactions(transactions);

  // Act & Assert
  const mojom::StatementInfoPtr expected_statement =
      mojom::StatementInfo::New();
  expected_statement->min_earnings_last_month =
      0.01 * kMinEstimatedEarningsMultiplier.Get();
  expected_statement->max_earnings_last_month = 0.01;
  expected_statement->min_earnings_this_month =
      0.05 * kMinEstimatedEarningsMultiplier.Get();
  expected_statement->max_earnings_this_month = 0.05;
  expected_statement->next_payment_date =
      test::TimeFromUTCString("7 January 2021 23:59:59.999");
  expected_statement->ads_received_this_month = 3;
  expected_statement->ads_summary_this_month = {
      {mojom::AdType::kNotificationAd, 3}};

  base::MockCallback<BuildStatementCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_statement))));
  BuildStatement(callback.Get());
}

TEST_F(BraveAdsStatementTest, GetForTransactionsSplitOverTwoYears) {
  // Arrange
  TransactionList transactions;

  AdvanceClockTo(test::TimeFromString("31 December 2020"));

  const TransactionInfo transaction_1 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_2);

  AdvanceClockTo(test::TimeFromString("1 January 2021"));

  const TransactionInfo transaction_3 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_4);

  const TransactionInfo transaction_5 = test::BuildUnreconciledTransaction(
      /*value=*/0.0, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kClicked,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_5);

  const TransactionInfo transaction_6 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_6);

  database::SaveTransactions(transactions);

  // Act & Assert
  const mojom::StatementInfoPtr expected_statement =
      mojom::StatementInfo::New();
  expected_statement->min_earnings_last_month =
      0.01 * kMinEstimatedEarningsMultiplier.Get();
  expected_statement->max_earnings_last_month = 0.01;
  expected_statement->min_earnings_this_month =
      0.04 * kMinEstimatedEarningsMultiplier.Get();
  expected_statement->max_earnings_this_month = 0.04;
  expected_statement->next_payment_date =
      test::TimeFromUTCString("7 January 2021 23:59:59.999");
  expected_statement->ads_received_this_month = 3;
  expected_statement->ads_summary_this_month = {
      {mojom::AdType::kNotificationAd, 3}};

  base::MockCallback<BuildStatementCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_statement))));
  BuildStatement(callback.Get());
}

TEST_F(BraveAdsStatementTest, GetForNoTransactions) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("18 November 2020"));

  // Act & Assert
  const mojom::StatementInfoPtr expected_statement =
      mojom::StatementInfo::New();
  expected_statement->min_earnings_last_month = 0.0;
  expected_statement->max_earnings_last_month = 0.0;
  expected_statement->min_earnings_this_month = 0.0;
  expected_statement->max_earnings_this_month = 0.0;
  expected_statement->next_payment_date =
      test::TimeFromUTCString("7 January 2021 23:59:59.999");
  expected_statement->ads_received_this_month = 0;
  expected_statement->ads_summary_this_month.clear();

  base::MockCallback<BuildStatementCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_statement))));
  BuildStatement(callback.Get());
}

TEST_F(BraveAdsStatementTest, GetWithFilteredTransactions) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("12 October 2020"));
  TransactionList transactions;

  const TransactionInfo transaction_1 = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_1);

  TransactionInfo transaction_2 = test::BuildTransaction(
      /*value=*/0.02, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transaction_2.ad_type = mojom::AdType::kNewTabPageAd;
  transactions.push_back(transaction_2);

  AdvanceClockTo(test::TimeFromString("18 November 2020"));

  const TransactionInfo transaction_3 = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_3);

  TransactionInfo transaction_4 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transaction_4.ad_type = mojom::AdType::kNewTabPageAd;
  transactions.push_back(transaction_4);

  database::SaveTransactions(transactions);

  // Act & Assert
  const mojom::StatementInfoPtr expected_statement =
      mojom::StatementInfo::New();
  expected_statement->min_earnings_last_month =
      0.01 * kMinEstimatedEarningsMultiplier.Get();
  expected_statement->max_earnings_last_month = 0.03;
  expected_statement->min_earnings_this_month =
      0.01 * kMinEstimatedEarningsMultiplier.Get();
  expected_statement->max_earnings_this_month = 0.02;
  expected_statement->next_payment_date =
      test::TimeFromUTCString("7 December 2020 23:59:59.999");
  expected_statement->ads_received_this_month = 2;
  expected_statement->ads_summary_this_month = {
      {mojom::AdType::kNotificationAd, 1}, {mojom::AdType::kNewTabPageAd, 1}};

  base::MockCallback<BuildStatementCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_statement))));
  BuildStatement(callback.Get());
}

}  // namespace brave_ads
