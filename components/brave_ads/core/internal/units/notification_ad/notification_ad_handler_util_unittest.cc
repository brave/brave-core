/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/units/notification_ad/notification_ad_handler_util.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsNotificationAdUtilTest : public UnitTestBase {};

TEST_F(BraveAdsNotificationAdUtilTest, CanServeIfUserIsActive) {
  // Act & Assert
  EXPECT_TRUE(CanServeIfUserIsActive());
}

TEST_F(BraveAdsNotificationAdUtilTest, CannotServeIfUserIsActive) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  // Act & Assert
  EXPECT_FALSE(CanServeIfUserIsActive());
}

TEST_F(BraveAdsNotificationAdUtilTest, ShouldServe) {
  // Act & Assert
  EXPECT_TRUE(ShouldServe());
}

TEST_F(BraveAdsNotificationAdUtilTest,
       ShouldNotServeIfOptedOutOfNotificationAds) {
  // Arrange
  OptOutOfNotificationAdsForTesting();

  // Act & Assert
  EXPECT_FALSE(ShouldServe());
}

TEST_F(BraveAdsNotificationAdUtilTest, CanServeAtRegularIntervals) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  // Act & Assert
  EXPECT_TRUE(CanServeAtRegularIntervals());
}

TEST_F(BraveAdsNotificationAdUtilTest, CannotServeAtRegularIntervals) {
  // Act & Assert
  EXPECT_FALSE(CanServeAtRegularIntervals());
}

}  // namespace brave_ads
