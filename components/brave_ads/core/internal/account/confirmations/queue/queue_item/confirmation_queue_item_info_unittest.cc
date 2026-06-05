/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/test/non_reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_builder.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/test/settings_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationQueueItemInfoTest : public test::TestBase {};

TEST_F(BraveAdsConfirmationQueueItemInfoTest, IsValid) {
  // Arrange
  test::DisableBraveRewards();

  std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  const ConfirmationQueueItemInfo queue_item =
      BuildConfirmationQueueItem(*confirmation, /*process_at=*/test::Now());

  // Act & Assert
  EXPECT_TRUE(queue_item.IsValid());
}

TEST_F(BraveAdsConfirmationQueueItemInfoTest, IsNotValidIfProcessAtIsAbsent) {
  // Arrange
  test::DisableBraveRewards();

  std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  ConfirmationQueueItemInfo queue_item =
      BuildConfirmationQueueItem(*confirmation, /*process_at=*/test::Now());
  queue_item.process_at = std::nullopt;

  // Act & Assert
  EXPECT_FALSE(queue_item.IsValid());
}

TEST_F(BraveAdsConfirmationQueueItemInfoTest,
       IsNotValidIfTransactionIdIsEmpty) {
  // Arrange
  test::DisableBraveRewards();

  std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  ConfirmationQueueItemInfo queue_item =
      BuildConfirmationQueueItem(*confirmation, /*process_at=*/test::Now());
  queue_item.confirmation.transaction_id = "";

  // Act & Assert
  EXPECT_FALSE(queue_item.IsValid());
}

TEST_F(BraveAdsConfirmationQueueItemInfoTest,
       IsNotValidIfCreativeInstanceIdIsEmpty) {
  // Arrange
  test::DisableBraveRewards();

  std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  ConfirmationQueueItemInfo queue_item =
      BuildConfirmationQueueItem(*confirmation, /*process_at=*/test::Now());
  queue_item.confirmation.creative_instance_id = "";

  // Act & Assert
  EXPECT_FALSE(queue_item.IsValid());
}

TEST_F(BraveAdsConfirmationQueueItemInfoTest,
       IsNotValidIfConfirmationTypeIsUndefined) {
  // Arrange
  test::DisableBraveRewards();

  std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  ConfirmationQueueItemInfo queue_item =
      BuildConfirmationQueueItem(*confirmation, /*process_at=*/test::Now());
  queue_item.confirmation.type = mojom::ConfirmationType::kUndefined;

  // Act & Assert
  EXPECT_FALSE(queue_item.IsValid());
}

TEST_F(BraveAdsConfirmationQueueItemInfoTest, IsNotValidIfAdTypeIsUndefined) {
  // Arrange
  test::DisableBraveRewards();

  std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  ConfirmationQueueItemInfo queue_item =
      BuildConfirmationQueueItem(*confirmation, /*process_at=*/test::Now());
  queue_item.confirmation.ad_type = mojom::AdType::kUndefined;

  // Act & Assert
  EXPECT_FALSE(queue_item.IsValid());
}

}  // namespace brave_ads
