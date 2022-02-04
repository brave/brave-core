/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/statement_util.h"

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/account/statement/ad_rewards_features.h"
#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/number_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsStatementUtilTest : public UnitTestBase {
 protected:
  BatAdsStatementUtilTest() = default;

  ~BatAdsStatementUtilTest() override = default;
};

TEST_F(BatAdsStatementUtilTest, GetNextPaymentDate) {
  // Arrange
  base::FieldTrialParams parameters;
  parameters["next_payment_day"] = "7";

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(features::kAdRewards,
                                                         parameters);

  AdvanceClock(TimeFromString("31 January 2020", /* is_local */ false));

  const base::Time& next_token_redemption_at =
      TimeFromString("5 February 2020", /* is_local */ false);
  AdsClientHelper::Get()->SetDoublePref(prefs::kNextTokenRedemptionAt,
                                        next_token_redemption_at.ToDoubleT());

  const TransactionList transactions;

  // Act
  const base::Time& next_payment_date = GetNextPaymentDate(transactions);

  // Assert
  const base::Time& expected_next_payment_date =
      TimeFromString("7 March 2020 23:59:59.999", /* is_local */ false);
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatAdsStatementUtilTest, GetEarningsForThisMonth) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.0, ConfirmationType::kClicked);
  transactions.push_back(transaction_2);

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.03, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  const TransactionInfo& transaction_4 =
      BuildTransaction(0.02, ConfirmationType::kViewed);
  transactions.push_back(transaction_4);

  // Act
  const double earnings = GetEarningsForThisMonth(transactions);

  // Assert
  const double expected_earnings = 0.05;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

TEST_F(BatAdsStatementUtilTest, GetUnreconciledEarningsForPreviousMonths) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const base::Time& reconciled_at = Now() + base::Days(1);
  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed, reconciled_at);
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.04, ConfirmationType::kViewed);
  transactions.push_back(transaction_2);

  AdvanceClock(TimeFromString("3 December 2020", /* is_local */ true));

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.0, ConfirmationType::kClicked);
  transactions.push_back(transaction_3);

  const TransactionInfo& transaction_4 =
      BuildTransaction(0.03, ConfirmationType::kViewed);
  transactions.push_back(transaction_4);

  const TransactionInfo& transaction_5 =
      BuildTransaction(0.02, ConfirmationType::kViewed);
  transactions.push_back(transaction_5);

  // Act
  const double earnings =
      GetUnreconciledEarningsForPreviousMonths(transactions);

  // Assert
  const double expected_earnings = 0.04;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

TEST_F(BatAdsStatementUtilTest, GetReconciledEarningsForLastMonth) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction_1);

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_2);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.0, ConfirmationType::kClicked);
  transactions.push_back(transaction_3);

  const TransactionInfo& transaction_4 =
      BuildTransaction(0.03, ConfirmationType::kViewed);
  transactions.push_back(transaction_4);

  const TransactionInfo& transaction_5 =
      BuildTransaction(0.02, ConfirmationType::kViewed);
  transactions.push_back(transaction_5);

  // Act
  const double earnings = GetReconciledEarningsForLastMonth(transactions);

  // Assert
  const double expected_earnings = 0.01;
  EXPECT_TRUE(DoubleEquals(expected_earnings, earnings));
}

TEST_F(BatAdsStatementUtilTest, GetAdsReceivedForThisMonth) {
  // Arrange
  AdvanceClock(TimeFromString("5 November 2020", /* is_local */ true));

  TransactionList transactions;

  const TransactionInfo& transaction_1 =
      BuildTransaction(0.01, ConfirmationType::kViewed);
  transactions.push_back(transaction_1);

  AdvanceClock(TimeFromString("25 December 2020", /* is_local */ true));

  const TransactionInfo& transaction_2 =
      BuildTransaction(0.0, ConfirmationType::kClicked);
  transactions.push_back(transaction_2);

  const TransactionInfo& transaction_3 =
      BuildTransaction(0.03, ConfirmationType::kViewed);
  transactions.push_back(transaction_3);

  const TransactionInfo& transaction_4 =
      BuildTransaction(0.02, ConfirmationType::kViewed);
  transactions.push_back(transaction_4);

  // Act
  const int ads_received = GetAdsReceivedForThisMonth(transactions);

  // Assert
  const int expected_ads_received = 2;
  EXPECT_EQ(expected_ads_received, ads_received);
}

}  // namespace ads
