/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/statement/next_payment_date_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/account/statement/statement_feature.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNextPaymentDateUtilTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        kAccountStatementFeature, {{"next_payment_day", "5"}});
  }

  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(BraveAdsNextPaymentDateUtilTest,
       TimeNowIsBeforeNextPaymentDayWithReconciledTransactionsPreviousMonth) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("1 January 2020"));

  TransactionList transactions;
  const TransactionInfo transaction = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction);

  AdvanceClockTo(test::TimeFromUTCString("1 February 2020"));

  const base::Time next_token_redemption_at =
      test::TimeFromUTCString("5 February 2020");

  // Act
  const base::Time next_payment_date =
      CalculateNextPaymentDate(next_token_redemption_at, transactions);

  // Assert
  EXPECT_EQ(test::TimeFromUTCString("5 February 2020 23:59:59.999"),
            next_payment_date);
}

TEST_F(BraveAdsNextPaymentDateUtilTest,
       TimeNowIsBeforeNextPaymentDayWithNoReconciledTransactionsPreviousMonth) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("1 February 2020"));

  const base::Time next_token_redemption_at =
      test::TimeFromUTCString("5 February 2020");

  // Act
  const base::Time next_payment_date =
      CalculateNextPaymentDate(next_token_redemption_at, /*transactions=*/{});

  // Assert
  EXPECT_EQ(test::TimeFromUTCString("5 March 2020 23:59:59.999"),
            next_payment_date);
}

TEST_F(BraveAdsNextPaymentDateUtilTest,
       TimeNowIsAfterNextPaymentDayWithReconciledTransactionsThisMonth) {
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("31 January 2020"));

  TransactionList transactions;
  const TransactionInfo transaction = test::BuildTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression, /*reconciled_at=*/test::Now(),
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction);

  const base::Time next_token_redemption_at =
      test::TimeFromUTCString("5 February 2020");

  // Act
  const base::Time next_payment_date =
      CalculateNextPaymentDate(next_token_redemption_at, transactions);

  // Act & Assert
  EXPECT_EQ(test::TimeFromUTCString("5 February 2020 23:59:59.999"),
            next_payment_date);
}

TEST_F(
    BraveAdsNextPaymentDateUtilTest,
    TimeNowIsAfterNextPaymentDayWhenNextTokenRedemptionDateIsThisMonthAndNoReconciledTransactionsThisMonth) {  // NOLINT
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("11 January 2020"));

  const base::Time next_token_redemption_at =
      test::TimeFromUTCString("31 January 2020");

  // Act
  const base::Time next_payment_date =
      CalculateNextPaymentDate(next_token_redemption_at, /*transactions=*/{});

  // Assert
  EXPECT_EQ(test::TimeFromUTCString("5 February 2020 23:59:59.999"),
            next_payment_date);
}

TEST_F(
    BraveAdsNextPaymentDateUtilTest,
    TimeNowIsAfterNextPaymentDayWhenNextTokenRedemptionDateIsNextMonthAndNoReconciledTransactionsThisMonth) {  // NOLINT
  // Arrange
  AdvanceClockTo(test::TimeFromUTCString("31 January 2020"));

  const base::Time next_token_redemption_at =
      test::TimeFromUTCString("5 February 2020");

  // Act
  const base::Time next_payment_date =
      CalculateNextPaymentDate(next_token_redemption_at, /*transactions=*/{});

  // Assert
  EXPECT_EQ(test::TimeFromUTCString("5 March 2020 23:59:59.999"),
            next_payment_date);
}

}  // namespace brave_ads
