/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/full_screen_mode_frequency_cap.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsFullScreenModeFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsFullScreenModeFrequencyCapTest() = default;

  ~BatAdsFullScreenModeFrequencyCapTest() override = default;
};

TEST_F(BatAdsFullScreenModeFrequencyCapTest, AllowAd) {
  // Arrange
  MockIsFullScreen(ads_client_mock_, false);

  // Act
  FullScreenModeFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsFullScreenModeFrequencyCapTest, AlwaysAllowAdForAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  MockIsFullScreen(ads_client_mock_, true);

  // Act
  FullScreenModeFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsFullScreenModeFrequencyCapTest, AlwaysAllowAdForIOS) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kIOS);

  MockIsFullScreen(ads_client_mock_, true);

  // Act
  FullScreenModeFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsFullScreenModeFrequencyCapTest, DoNotAllowAd) {
  // Arrange
  MockIsFullScreen(ads_client_mock_, true);

  // Act
  FullScreenModeFrequencyCap frequency_cap;
  const bool is_allowed = frequency_cap.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
