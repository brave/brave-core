/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations.h"

#include <memory>

#include "base/test/test_future.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_database_table.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_info.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationsTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    confirmations_ = std::make_unique<Confirmations>();
  }

  std::unique_ptr<Confirmations> confirmations_;

  database::table::ConfirmationQueue confirmation_queue_database_table_;
};

TEST_F(BraveAdsConfirmationsTest, ConfirmForRewardsUser) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*use_random_uuids=*/false);

  // Act
  confirmations_->Confirm(transaction, /*user_data=*/{});

  // Assert
  base::test::TestFuture<bool, ConfirmationQueueItemList> test_future;
  confirmation_queue_database_table_.GetAll(
      test_future.GetCallback<bool, const ConfirmationQueueItemList&>());
  const auto [success, confirmation_queue_items] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(confirmation_queue_items, ::testing::SizeIs(1));
}

TEST_F(BraveAdsConfirmationsTest, ConfirmForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, mojom::AdType::kNotificationAd,
      mojom::ConfirmationType::kViewedImpression,
      /*use_random_uuids=*/false);

  // Act
  confirmations_->Confirm(transaction, /*user_data=*/{});

  // Assert
  base::test::TestFuture<bool, ConfirmationQueueItemList> test_future;
  confirmation_queue_database_table_.GetAll(
      test_future.GetCallback<bool, const ConfirmationQueueItemList&>());
  const auto [success, confirmation_queue_items] = test_future.Take();
  EXPECT_TRUE(success);
  EXPECT_THAT(confirmation_queue_items, ::testing::SizeIs(1));
}

}  // namespace brave_ads
