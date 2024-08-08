/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_builder.h"

#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationQueueItemBuilderTest : public test::TestBase {};

TEST_F(BraveAdsConfirmationQueueItemBuilderTest,
       BuildRewardConfirmationQueueItem) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  // Act
  const ConfirmationQueueItemInfo confirmation_queue_item =
      BuildConfirmationQueueItem(*confirmation, /*process_at=*/test::Now());

  // Assert
  EXPECT_THAT(confirmation_queue_item,
              ::testing::FieldsAre(*confirmation, /*process_at*/ test::Now(),
                                   /*retry_count*/ 0));
}

TEST_F(BraveAdsConfirmationQueueItemBuilderTest,
       BuildNonRewardConfirmationQueueItem) {
  // Arrange
  test::DisableBraveRewards();

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  // Act
  const ConfirmationQueueItemInfo confirmation_queue_item =
      BuildConfirmationQueueItem(*confirmation, /*process_at=*/test::Now());

  // Assert
  EXPECT_THAT(confirmation_queue_item,
              ::testing::FieldsAre(*confirmation, /*process_at*/ test::Now(),
                                   /*retry_count*/ 0));
}

}  // namespace brave_ads
