/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/statement/ads_summary_util.h"

#include "base/containers/flat_map.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdsSummaryUtilTest : public UnitTestBase {};

TEST_F(BraveAdsAdsSummaryUtilTest, GetAdsSummaryForDateRange) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local=*/true));

  TransactionList transactions;

  const TransactionInfo transaction_1 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_1);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local=*/true));

  const TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.0, ConfirmationType::kClicked,
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_2);

  const TransactionInfo transaction_3 = test::BuildUnreconciledTransaction(
      /*value=*/0.03, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_3);

  const base::Time from_time = Now();

  AdvanceClockTo(TimeFromString("1 January 2021", /*is_local=*/true));

  const TransactionInfo transaction_4 = test::BuildUnreconciledTransaction(
      /*value=*/0.02, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_4);

  TransactionInfo transaction_5 = test::BuildUnreconciledTransaction(
      /*value=*/0.02, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/true);
  transaction_5.ad_type = AdType::kNewTabPageAd;
  transactions.push_back(transaction_5);

  TransactionInfo transaction_6 = test::BuildUnreconciledTransaction(
      /*value=*/0.02, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/true);
  transaction_6.ad_type = AdType::kInlineContentAd;
  transactions.push_back(transaction_6);

  // Act & Assert
  const base::flat_map<std::string, int32_t> expected_ads_summary = {
      {"ad_notification", 2}, {"new_tab_page_ad", 1}, {"inline_content_ad", 1}};
  EXPECT_EQ(
      expected_ads_summary,
      GetAdsSummaryForDateRange(transactions, from_time, DistantFuture()));
}

TEST_F(BraveAdsAdsSummaryUtilTest, DoNotGetAdsSummaryForDateRange) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local=*/true));

  TransactionList transactions;

  const TransactionInfo transaction_1 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.0, ConfirmationType::kClicked,
      /*should_use_random_uuids=*/true);
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("1 January 2021", /*is_local=*/true));

  // Act
  const base::flat_map<std::string, int32_t> ads_summary =
      GetAdsSummaryForDateRange(transactions, Now(), DistantFuture());

  // Assert
  EXPECT_TRUE(ads_summary.empty());
}

TEST_F(BraveAdsAdsSummaryUtilTest, GetAdsSummaryForNoTransactions) {
  // Arrange
  const TransactionList transactions;

  // Act
  const base::flat_map<std::string, int32_t> ads_summary =
      GetAdsSummaryForDateRange(transactions, DistantPast(), DistantFuture());

  // Assert
  EXPECT_TRUE(ads_summary.empty());
}

}  // namespace brave_ads
