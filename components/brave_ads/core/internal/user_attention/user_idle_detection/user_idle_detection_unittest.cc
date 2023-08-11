/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_attention/user_idle_detection/user_idle_detection.h"

#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsUserIdleDetectionTest : public UnitTestBase {};

TEST_F(BraveAdsUserIdleDetectionTest, RewardsUserDidBecomeActive) {
  // Arrange
  const UserIdleDetection user_idle_detection;
  EXPECT_CALL(ads_client_mock_, Log);

  // Act
  NotifyUserDidBecomeActive(/*idle_time*/ base::Seconds(10),
                            /*screen_was_locked*/ false);

  // Assert
}

TEST_F(BraveAdsUserIdleDetectionTest, NonRewardsUserDidBecomeActive) {
  // Arrange
  DisableBraveRewardsForTesting();

  const UserIdleDetection user_idle_detection;
  EXPECT_CALL(ads_client_mock_, Log).Times(0);

  // Act
  NotifyUserDidBecomeActive(/*idle_time*/ base::Seconds(10),
                            /*screen_was_locked*/ false);

  // Assert
}

TEST_F(BraveAdsUserIdleDetectionTest,
       RewardsUserDidBecomeActiveWhileScreenWasLocked) {
  // Arrange
  const UserIdleDetection user_idle_detection;
  EXPECT_CALL(ads_client_mock_, Log).Times(2);

  // Act
  NotifyUserDidBecomeActive(/*idle_time*/ base::Seconds(10),
                            /*screen_was_locked*/ true);

  // Assert
}

TEST_F(BraveAdsUserIdleDetectionTest,
       NonRewardsUserDidBecomeActiveWhileScreenWasLocked) {
  // Arrange
  DisableBraveRewardsForTesting();

  const UserIdleDetection user_idle_detection;
  EXPECT_CALL(ads_client_mock_, Log).Times(0);

  // Act
  NotifyUserDidBecomeActive(/*idle_time*/ base::Seconds(10),
                            /*screen_was_locked*/ true);

  // Assert
}

TEST_F(BraveAdsUserIdleDetectionTest, RewardsUserDidBecomeIdle) {
  // Arrange
  const UserIdleDetection user_idle_detection;
  EXPECT_CALL(ads_client_mock_, Log);

  // Act
  NotifyUserDidBecomeIdle();

  // Assert
}

TEST_F(BraveAdsUserIdleDetectionTest, NonRewardsUserDidBecomeIdle) {
  // Arrange
  DisableBraveRewardsForTesting();

  const UserIdleDetection user_idle_detection;
  EXPECT_CALL(ads_client_mock_, Log).Times(0);

  // Act
  NotifyUserDidBecomeIdle();

  // Assert
}

}  // namespace brave_ads
