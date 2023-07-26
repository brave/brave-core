/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/statement/statement_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/account/statement/statement_feature.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsStatementUtilTest : public UnitTestBase {};

TEST_F(BraveAdsStatementUtilTest, GetNextPaymentDate) {
  // Arrange
  base::FieldTrialParams params;
  params["next_payment_day"] = "7";
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kAccountStatementFeature, params);

  AdvanceClockTo(TimeFromString("31 January 2020", /*is_local*/ false));

  const base::Time next_token_redemption_at =
      TimeFromString("5 February 2020", /*is_local*/ false);
  SetDefaultTimePref(prefs::kNextTokenRedemptionAt, next_token_redemption_at);

  const TransactionList transactions;

  // Act
  const base::Time next_payment_date = GetNextPaymentDate(transactions);

  // Assert
  const base::Time expected_next_payment_date =
      TimeFromString("7 March 2020 23:59:59.999", /*is_local*/ false);
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BraveAdsStatementUtilTest, GetEstimatedEarningsForThisMonth) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 =
      BuildUnreconciledTransaction(/*value*/ 0.02, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = BuildTransaction(
      /*value*/ 0.01, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_3 =
      BuildUnreconciledTransaction(/*value*/ 0.0, ConfirmationType::kClicked,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 =
      BuildUnreconciledTransaction(/*value*/ 0.03, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_4);

  const TransactionInfo transaction_5 =
      BuildUnreconciledTransaction(/*value*/ 0.02, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_5);

  TransactionInfo transaction_6 =
      BuildUnreconciledTransaction(/*value*/ 0.02, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transaction_6.ad_type = AdType::kNewTabPageAd;
  transactions.push_back(transaction_6);

  // Act
  const auto [min, max] = GetEstimatedEarningsForThisMonth(transactions);

  // Assert
  EXPECT_DOUBLE_EQ(0.07 * kMinEstimatedEarningsMultiplier.Get(), min);
  EXPECT_DOUBLE_EQ(0.09, max);
}

TEST_F(BraveAdsStatementUtilTest, GetEstimatedEarningsForLastMonth) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 = BuildTransaction(
      /*value*/ 0.02, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  TransactionInfo transaction_2 = BuildTransaction(
      /*value*/ 0.02, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transaction_2.ad_type = AdType::kNewTabPageAd;
  transactions.push_back(transaction_2);

  const TransactionInfo transaction_3 =
      BuildUnreconciledTransaction(/*value*/ 0.01, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_3);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_4 =
      BuildUnreconciledTransaction(/*value*/ 0.0, ConfirmationType::kClicked,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_4);

  const TransactionInfo transaction_5 = BuildTransaction(
      /*value*/ 0.03, ConfirmationType::kViewed, /*reconciled_at*/ Now(),
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_5);

  // Act
  const auto [min, max] = GetEstimatedEarningsForLastMonth(transactions);

  // Assert
  EXPECT_DOUBLE_EQ(0.02 * kMinEstimatedEarningsMultiplier.Get(), min);
  EXPECT_DOUBLE_EQ(0.04, max);
}

TEST_F(BraveAdsStatementUtilTest, GetAdTypesReceivedThisMonth) {
  // Arrange
  AdvanceClockTo(TimeFromString("5 November 2020", /*is_local*/ true));

  TransactionList transactions;

  const TransactionInfo transaction_1 =
      BuildUnreconciledTransaction(/*value*/ 0.01, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  AdvanceClockTo(TimeFromString("25 December 2020", /*is_local*/ true));

  const TransactionInfo transaction_2 =
      BuildUnreconciledTransaction(/*value*/ 0.0, ConfirmationType::kClicked,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  const TransactionInfo transaction_3 =
      BuildUnreconciledTransaction(/*value*/ 0.03, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_3);

  const TransactionInfo transaction_4 =
      BuildUnreconciledTransaction(/*value*/ 0.02, ConfirmationType::kViewed,
                                   /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_4);

  // Act
  auto result = GetAdTypesReceivedThisMonth(transactions);

  // Assert
  EXPECT_EQ(1U, result.size());
  EXPECT_EQ(2, result["ad_notification"]);
}

}  // namespace brave_ads
