/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_builder.h"

#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_mock.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationQueueItemBuilderTest : public UnitTestBase {
 protected:
  TokenGeneratorMock token_generator_mock_;
};

TEST_F(BraveAdsConfirmationQueueItemBuilderTest,
       BuildRewardConfirmationQueueItem) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(&token_generator_mock_,
                                    /*should_use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  // Act & Assert
  ConfirmationQueueItemInfo expected_confirmation_queue_item;
  expected_confirmation_queue_item.confirmation = *confirmation;
  expected_confirmation_queue_item.process_at = Now();
  expected_confirmation_queue_item.retry_count = 0;
  EXPECT_EQ(expected_confirmation_queue_item,
            BuildConfirmationQueueItem(*confirmation, /*process_at=*/Now()));
}

TEST_F(BraveAdsConfirmationQueueItemBuilderTest,
       BuildNonRewardConfirmationQueueItem) {
  // Arrange
  test::DisableBraveRewards();

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*should_use_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  // Act & Assert
  ConfirmationQueueItemInfo expected_confirmation_queue_item;
  expected_confirmation_queue_item.confirmation = *confirmation;
  expected_confirmation_queue_item.process_at = Now();
  expected_confirmation_queue_item.retry_count = 0;
  EXPECT_EQ(expected_confirmation_queue_item,
            BuildConfirmationQueueItem(*confirmation, /*process_at=*/Now()));
}

}  // namespace brave_ads
