/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"

#include <optional>

#include "brave/components/brave_ads/core/internal/account/confirmations/confirmation_info.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/non_reward/non_reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_database_table.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/queue_item/confirmation_queue_item_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/reward/reward_confirmation_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/token_generator_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationsUtilTest : public test::TestBase {
 protected:
  database::table::ConfirmationQueue confirmation_queue_database_table_;
};

TEST_F(BraveAdsConfirmationsUtilTest, IsRewardConfirmationValid) {
  // Arrange
  test::MockTokenGenerator(/*count=*/1);
  test::RefillConfirmationTokens(/*count=*/1);

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  // Act & Assert
  EXPECT_TRUE(IsValid(*confirmation));
}

TEST_F(BraveAdsConfirmationsUtilTest, IsNonRewardConfirmationValid) {
  // Arrange
  test::DisableBraveRewards();

  const std::optional<ConfirmationInfo> confirmation =
      test::BuildNonRewardConfirmation(/*should_generate_random_uuids=*/false);
  ASSERT_TRUE(confirmation);

  // Act & Assert
  EXPECT_TRUE(IsValid(*confirmation));
}

TEST_F(BraveAdsConfirmationsUtilTest, IsConfirmationNotValid) {
  // Arrange
  const ConfirmationInfo confirmation;

  // Act & Assert
  EXPECT_FALSE(IsValid(confirmation));
}

}  // namespace brave_ads
