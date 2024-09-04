/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions.h"

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTransactionsTest : public test::TestBase {};

TEST_F(BraveAdsTransactionsTest, Add) {
  // Arrange
  base::MockCallback<AddTransactionCallback> add_transaction_callback;
  EXPECT_CALL(add_transaction_callback,
              Run(/*success=*/true, /*transaction=*/::testing::_));

  // Act
  const TransactionInfo transaction =
      AddTransaction(test::kCreativeInstanceId, test::kSegment, /*value=*/0.01,
                     mojom::AdType::kNotificationAd,
                     mojom::ConfirmationType::kViewedImpression,
                     add_transaction_callback.Get());

  // Assert
  base::MockCallback<database::table::GetTransactionsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true, TransactionList{transaction}));
  const database::table::Transactions database_table;
  database_table.GetForDateRange(/*from_time=*/test::DistantPast(),
                                 /*to_time=*/test::DistantFuture(),
                                 callback.Get());
}

TEST_F(BraveAdsTransactionsTest, GetForDateRange) {
  // Arrange
  TransactionList transactions;

  AdvanceClockTo(test::TimeFromString("31 August 2019"));

  const TransactionInfo transaction_1 = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_1);

  AdvanceClockTo(
      test::TimeFromUTCString("11 September 2019"));  // A legendary moment.

  const TransactionInfo transaction_2 = test::BuildUnreconciledTransaction(
      /*value=*/0.0, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kDismissed,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_2);

  const TransactionInfo transaction_3 = test::BuildUnreconciledTransaction(
      /*value=*/0.0, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kClicked,
      /*should_generate_random_uuids=*/true);
  transactions.push_back(transaction_3);

  database::SaveTransactions(transactions);

  // Act & Assert
  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true,
                            TransactionList{transaction_2, transaction_3}));
  GetTransactionsForDateRange(/*from_time=*/test::Now(),
                              /*to_time=*/test::DistantFuture(),
                              callback.Get());
}

}  // namespace brave_ads
