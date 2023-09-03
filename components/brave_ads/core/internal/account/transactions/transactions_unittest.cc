/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/transactions/transactions.h"

#include <utility>

#include "base/test/mock_callback.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTransactionsTest : public UnitTestBase {};

TEST_F(BraveAdsTransactionsTest, Add) {
  // Arrange

  // Assert
  base::MockCallback<AddTransactionCallback> callback;
  EXPECT_CALL(callback, Run(/*success*/ true, ::testing::_));

  // Act
  const TransactionInfo transaction = AddTransaction(
      kCreativeInstanceId, kSegment, /*value*/ 0.01, AdType::kNotificationAd,
      ConfirmationType::kViewed, callback.Get());
}

TEST_F(BraveAdsTransactionsTest, GetForDateRange) {
  // Arrange
  TransactionList transactions;

  AdvanceClockTo(TimeFromString("31 October 2020", /*is_local*/ true));

  const TransactionInfo transaction_1 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  AdvanceClockTo(TimeFromString("18 November 2020", /*is_local*/ true));

  const TransactionInfo transaction_2 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.0, ConfirmationType::kDismissed,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  const TransactionInfo transaction_3 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.0, ConfirmationType::kClicked,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_3);

  SaveTransactionsForTesting(transactions);

  // Assert
  const TransactionList expected_transactions = {transaction_2, transaction_3};
  base::MockCallback<GetTransactionsCallback> callback;
  EXPECT_CALL(callback, Run(/*success*/ true, expected_transactions));

  // Act
  GetTransactionsForDateRange(Now(), DistantFuture(), callback.Get());
}

TEST_F(BraveAdsTransactionsTest, RemoveAll) {
  // Arrange
  TransactionList transactions;

  const TransactionInfo transaction_1 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.01, ConfirmationType::kViewed,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_1);

  const TransactionInfo transaction_2 = BuildUnreconciledTransactionForTesting(
      /*value*/ 0.0, ConfirmationType::kDismissed,
      /*should_use_random_uuids*/ true);
  transactions.push_back(transaction_2);

  SaveTransactionsForTesting(transactions);

  // Assert
  base::MockCallback<RemoveAllTransactionsCallback> callback;
  EXPECT_CALL(callback, Run(/*success*/ true));

  // Act
  RemoveAllTransactions(callback.Get());
}

}  // namespace brave_ads
