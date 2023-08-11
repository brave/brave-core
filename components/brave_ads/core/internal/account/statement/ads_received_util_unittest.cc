/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/statement/ads_received_util.h"

#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsAdsReceivedUtilTest : public UnitTestBase {};

TEST_F(BraveAdsAdsReceivedUtilTest, GetAdTypesReceivedForDateRange) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_2 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.0, ConfirmationType::kClicked,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  const TransactionInfo transaction_3 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.03, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_3);

  const base::Time from_time = Now();

  AdvanceClockTo(TimeFromString("1 January 2021", /*is_local*/ true));

  const TransactionInfo transaction_4 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.02, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_4);

  TransactionInfo transaction_5 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.02, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  transaction_5.ad_type = AdType::kNewTabPageAd;
  transactions.push_back(transaction_5);

  TransactionInfo transaction_6 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.02, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  transaction_6.ad_type = AdType::kInlineContentAd;
  transactions.push_back(transaction_6);

  // Act
  auto result =
      GetAdTypesReceivedForDateRange(transactions, from_time, DistantFuture());

  // Assert
  EXPECT_EQ(3U, result.size());
  EXPECT_EQ(2, result["ad_notification"]);
  EXPECT_EQ(1, result["new_tab_page_ad"]);
  EXPECT_EQ(1, result["inline_content_ad"]);
}

TEST_F(BraveAdsAdsReceivedUtilTest, DoNotGetAdsReceivedForDateRange) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.0, ConfirmationType::kClicked,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("1 January 2021", /*is_local*/ true));

  // Act
  auto result =
      GetAdTypesReceivedForDateRange(transactions, Now(), DistantFuture());

  // Assert
  EXPECT_TRUE(result.empty());
}

TEST_F(BraveAdsAdsReceivedUtilTest, GetAdTypesReceivedForNoTransactions) {
  // Arrange
  const TransactionList transactions;

  // Act
  auto result = GetAdTypesReceivedForDateRange(transactions, DistantPast(),
                                               DistantFuture());

  // Assert
  EXPECT_TRUE(result.empty());
}

}  // namespace brave_ads
