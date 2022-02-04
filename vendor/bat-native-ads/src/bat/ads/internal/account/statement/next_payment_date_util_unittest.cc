/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/statement/next_payment_date_util.h"

#include "base/feature_list.h"
#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/account/statement/ad_rewards_features.h"
#include "bat/ads/internal/account/transactions/transactions_unittest_util.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsNextPaymentDateUtilTest : public UnitTestBase {
 protected:
  BatAdsNextPaymentDateUtilTest() = default;

  ~BatAdsNextPaymentDateUtilTest() override = default;
};

TEST_F(BatAdsNextPaymentDateUtilTest,
       TimeNowIsBeforeNextPaymentDayWithReconciledTransactionsLastMonth) {
  // Arrange
  base::FieldTrialParams parameters;
  parameters["next_payment_day"] = "5";

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(features::kAdRewards,
                                                         parameters);

  AdvanceClock(TimeFromString("1 January 2020", /* is_local */ false));

  TransactionList transactions;
  const TransactionInfo& transaction =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction);

  AdvanceClock(TimeFromString("1 February 2020", /* is_local */ false));

  const base::Time& next_token_redemption_at =
      TimeFromString("5 February 2020", /* is_local */ false);

  // Act
  const base::Time& next_payment_date =
      CalculateNextPaymentDate(next_token_redemption_at, transactions);

  // Assert
  const base::Time& expected_next_payment_date =
      TimeFromString("5 February 2020 23:59:59.999", /* is_local */ false);
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatAdsNextPaymentDateUtilTest,
       TimeNowIsBeforeNextPaymentDayWithNoReconciledTransactionsLastMonth) {
  // Arrange
  base::FieldTrialParams parameters;
  parameters["next_payment_day"] = "5";

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(features::kAdRewards,
                                                         parameters);

  AdvanceClock(TimeFromString("1 February 2020", /* is_local */ false));

  const TransactionList transactions;

  const base::Time& next_token_redemption_at =
      TimeFromString("5 February 2020", /* is_local */ false);

  // Act
  const base::Time& next_payment_date =
      CalculateNextPaymentDate(next_token_redemption_at, transactions);

  // Assert
  const base::Time& expected_next_payment_date =
      TimeFromString("5 March 2020 23:59:59.999", /* is_local */ false);
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(BatAdsNextPaymentDateUtilTest,
       TimeNowIsAfterNextPaymentDayWithReconciledTransactionsThisMonth) {
  // Arrange
  base::FieldTrialParams parameters;
  parameters["next_payment_day"] = "5";

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(features::kAdRewards,
                                                         parameters);

  AdvanceClock(TimeFromString("31 January 2020", /* is_local */ false));

  TransactionList transactions;
  const TransactionInfo& transaction =
      BuildTransaction(0.01, ConfirmationType::kViewed, Now());
  transactions.push_back(transaction);

  const base::Time& next_token_redemption_at =
      TimeFromString("5 February 2020", /* is_local */ false);

  // Act
  const base::Time& next_payment_date =
      CalculateNextPaymentDate(next_token_redemption_at, transactions);

  // Assert
  const base::Time& expected_next_payment_date =
      TimeFromString("5 February 2020 23:59:59.999", /* is_local */ false);
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(
    BatAdsNextPaymentDateUtilTest,
    TimeNowIsAfterNextPaymentDayWhenNextTokenRedemptionDateIsThisMonthAndNoReconciledTransactionsThisMonth) {  // NOLINT
  // Arrange
  base::FieldTrialParams parameters;
  parameters["next_payment_day"] = "5";

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(features::kAdRewards,
                                                         parameters);

  AdvanceClock(TimeFromString("11 January 2020", /* is_local */ false));

  const TransactionList transactions;

  const base::Time& next_token_redemption_at =
      TimeFromString("31 January 2020", /* is_local */ false);

  // Act
  const base::Time& next_payment_date =
      CalculateNextPaymentDate(next_token_redemption_at, transactions);

  // Assert
  const base::Time& expected_next_payment_date =
      TimeFromString("5 February 2020 23:59:59.999", /* is_local */ false);
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

TEST_F(
    BatAdsNextPaymentDateUtilTest,
    TimeNowIsAfterNextPaymentDayWhenNextTokenRedemptionDateIsNextMonthAndNoReconciledTransactionsThisMonth) {  // NOLINT
  // Arrange
  base::FieldTrialParams parameters;
  parameters["next_payment_day"] = "5";

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(features::kAdRewards,
                                                         parameters);

  AdvanceClock(TimeFromString("31 January 2020", /* is_local */ false));

  const TransactionList transactions;

  const base::Time& next_token_redemption_at =
      TimeFromString("5 February 2020", /* is_local */ false);

  // Act
  const base::Time& next_payment_date =
      CalculateNextPaymentDate(next_token_redemption_at, transactions);

  // Assert
  const base::Time& expected_next_payment_date =
      TimeFromString("5 March 2020 23:59:59.999", /* is_local */ false);
  EXPECT_EQ(expected_next_payment_date, next_payment_date);
}

}  // namespace ads
