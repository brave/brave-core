/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_util.h"

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_util_constants.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationQueueItemDelayTest : public test::TestBase {
 protected:
  TokenGeneratorMock token_generator_mock_;
};

TEST_F(BraveAdsConfirmationQueueItemDelayTest,
       CalculateDelayBeforeProcessingConfirmationQueueItem) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(&token_generator_mock_,
                                    /*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);
  const ConfirmationQueueItemInfo confirmation_queue_item =
      BuildConfirmationQueueItem(*confirmation,
                                 /*process_at=*/test::Now() + base::Hours(1));

  // Act
  const base::TimeDelta delay_before_processing_confirmation_queue_item =
      CalculateDelayBeforeProcessingConfirmationQueueItem(
          confirmation_queue_item);

  // Assert
  EXPECT_EQ(base::Hours(1), delay_before_processing_confirmation_queue_item);
}

TEST_F(BraveAdsConfirmationQueueItemDelayTest,
       CalculateDelayBeforeProcessingPastDueConfirmationQueueItem) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(&token_generator_mock_,
                                    /*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);
  const ConfirmationQueueItemInfo confirmation_queue_item =
      BuildConfirmationQueueItem(*confirmation,
                                 /*process_at=*/test::DistantPast());

  // Act
  const base::TimeDelta delay_before_processing_confirmation_queue_item =
      CalculateDelayBeforeProcessingConfirmationQueueItem(
          confirmation_queue_item);

  // Assert
  EXPECT_EQ(kMinimumDelayBeforeProcessingConfirmationQueueItem,
            delay_before_processing_confirmation_queue_item);
}

TEST_F(BraveAdsConfirmationQueueItemDelayTest,
       CalculateMinimumDelayBeforeProcessingConfirmationQueueItem) {
  // Arrange
  test::MockTokenGenerator(token_generator_mock_, /*count=*/1);

  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(&token_generator_mock_,
                                    /*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);
  const ConfirmationQueueItemInfo confirmation_queue_item =
      BuildConfirmationQueueItem(
          *confirmation,
          /*process_at=*/test::Now() + base::Milliseconds(1));

  // Act
  const base::TimeDelta delay_before_processing_confirmation_queue_item =
      CalculateDelayBeforeProcessingConfirmationQueueItem(
          confirmation_queue_item);

  // Assert
  EXPECT_EQ(kMinimumDelayBeforeProcessingConfirmationQueueItem,
            delay_before_processing_confirmation_queue_item);
}

}  // namespace brave_ads
