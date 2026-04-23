/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/settings/test/settings_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsUserIdleDetectionTest : public test::TestBase {};

TEST_F(BraveAdsUserIdleDetectionTest, RewardsUserDidBecomeActive) {
  // Act & Assert
  EXPECT_CALL(ads_client_mock_, Log).Times(::testing::AnyNumber());
  EXPECT_CALL(ads_client_mock_,
              Log(/*file=*/::testing::_, /*line=*/::testing::_,
                  /*verbose_level=*/::testing::_, "User is active after 10 s"));
  ads_client_notifier_.NotifyUserDidBecomeActive(
      /*idle_time=*/base::Seconds(10),
      /*screen_was_locked=*/false);
}

TEST_F(BraveAdsUserIdleDetectionTest, NonRewardsUserDidBecomeActive) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_CALL(ads_client_mock_, Log).Times(0);
  ads_client_notifier_.NotifyUserDidBecomeActive(
      /*idle_time=*/base::Seconds(10),
      /*screen_was_locked=*/false);
}

TEST_F(BraveAdsUserIdleDetectionTest,
       RewardsUserDidBecomeActiveWhileScreenWasLocked) {
  // Act & Assert
  EXPECT_CALL(ads_client_mock_, Log).Times(::testing::AnyNumber());
  EXPECT_CALL(ads_client_mock_,
              Log(/*file=*/::testing::_, /*line=*/::testing::_,
                  /*verbose_level=*/::testing::_, "User is active after 10 s"));
  EXPECT_CALL(ads_client_mock_,
              Log(/*file=*/::testing::_, /*line=*/::testing::_,
                  /*verbose_level=*/::testing::_,
                  "Screen was locked before the user become active"));
  ads_client_notifier_.NotifyUserDidBecomeActive(
      /*idle_time=*/base::Seconds(10),
      /*screen_was_locked=*/true);
}

TEST_F(BraveAdsUserIdleDetectionTest,
       NonRewardsUserDidBecomeActiveWhileScreenWasLocked) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_CALL(ads_client_mock_, Log).Times(0);
  ads_client_notifier_.NotifyUserDidBecomeActive(
      /*idle_time=*/base::Seconds(10),
      /*screen_was_locked=*/true);
}

TEST_F(BraveAdsUserIdleDetectionTest, RewardsUserDidBecomeIdle) {
  // Act & Assert
  EXPECT_CALL(ads_client_mock_, Log);
  ads_client_notifier_.NotifyUserDidBecomeIdle();
}

TEST_F(BraveAdsUserIdleDetectionTest, NonRewardsUserDidBecomeIdle) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_CALL(ads_client_mock_, Log).Times(0);
  ads_client_notifier_.NotifyUserDidBecomeIdle();
}

}  // namespace brave_ads
