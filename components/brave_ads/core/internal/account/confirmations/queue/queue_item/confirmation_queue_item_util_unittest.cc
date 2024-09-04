/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_builder.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_util_constants.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transaction_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationQueueItemDelayTest : public test::TestBase {};

TEST_F(BraveAdsConfirmationQueueItemDelayTest,
       CalculateDelayBeforeProcessingConfirmationQueueItem) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(/*should_generate_random_uuids=*/false);
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
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(/*should_generate_random_uuids=*/false);
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
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(/*should_generate_random_uuids=*/false);
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

TEST_F(BraveAdsConfirmationQueueItemDelayTest,
       RebuildConfirmationWithoutDynamicUserData) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  test::MockConfirmationUserData();

  AdvanceClockTo(test::TimeFromUTCString("Mon, 8 Jul 1996 09:25"));

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  const base::Time created_at = test::Now();

  AdvanceClockBy(base::Hours(1));

  // Act
  const ConfirmationInfo rebuilt_confirmation =
      RebuildConfirmationWithoutDynamicUserData(*confirmation);

  // Assert
  const RewardInfo expected_reward = test::BuildReward(rebuilt_confirmation);

  UserDataInfo expected_user_data = confirmation->user_data;
  expected_user_data.dynamic.clear();

  EXPECT_THAT(
      rebuilt_confirmation,
      ::testing::FieldsAre(test::kTransactionId, test::kCreativeInstanceId,
                           mojom::ConfirmationType::kViewedImpression,
                           mojom::AdType::kNotificationAd, created_at,
                           expected_reward, expected_user_data));
}

TEST_F(BraveAdsConfirmationQueueItemDelayTest,
       RebuildConfirmationDynamicUserData) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  test::MockConfirmationUserData();

  AdvanceClockTo(test::TimeFromUTCString("Mon, 8 Jul 1996 09:25"));

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  const base::Time created_at = test::Now();

  AdvanceClockBy(base::Hours(1));

  // Act
  const ConfirmationInfo rebuilt_confirmation =
      RebuildConfirmationDynamicUserData(*confirmation);

  // Assert
  const RewardInfo expected_reward = test::BuildReward(rebuilt_confirmation);

  UserDataInfo expected_user_data = rebuilt_confirmation.user_data;
  expected_user_data.dynamic = base::test::ParseJsonDict(
      R"(
          {
            "diagnosticId": "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2",
            "systemTimestamp": "1996-07-08T10:00:00.000Z"
          })");

  EXPECT_THAT(
      rebuilt_confirmation,
      ::testing::FieldsAre(test::kTransactionId, test::kCreativeInstanceId,
                           mojom::ConfirmationType::kViewedImpression,
                           mojom::AdType::kNotificationAd, created_at,
                           expected_reward, expected_user_data));
}

}  // namespace brave_ads
