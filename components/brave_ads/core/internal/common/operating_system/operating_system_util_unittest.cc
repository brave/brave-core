/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system_util.h"

#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system.h"
#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system_types.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsOperatingSystemUtilTest : public test::TestBase {};

TEST_F(BraveAdsOperatingSystemUtilTest, IsMobileOnAndroid) {
  // Arrange
  fake_operating_system_.SetType(OperatingSystemType::kAndroid);
  OperatingSystem::SetForTesting(&fake_operating_system_);

  // Act & Assert
  EXPECT_TRUE(IsMobile());
}

TEST_F(BraveAdsOperatingSystemUtilTest, IsMobileOnIOS) {
  // Arrange
  fake_operating_system_.SetType(OperatingSystemType::kIOS);
  OperatingSystem::SetForTesting(&fake_operating_system_);

  // Act & Assert
  EXPECT_TRUE(IsMobile());
}

TEST_F(BraveAdsOperatingSystemUtilTest, IsNotMobileOnLinux) {
  // Arrange
  fake_operating_system_.SetType(OperatingSystemType::kLinux);
  OperatingSystem::SetForTesting(&fake_operating_system_);

  // Act & Assert
  EXPECT_FALSE(IsMobile());
}

TEST_F(BraveAdsOperatingSystemUtilTest, IsNotMobileOnMacOS) {
  // Arrange
  fake_operating_system_.SetType(OperatingSystemType::kMacOS);
  OperatingSystem::SetForTesting(&fake_operating_system_);

  // Act & Assert
  EXPECT_FALSE(IsMobile());
}

TEST_F(BraveAdsOperatingSystemUtilTest, IsNotMobileOnWindows) {
  // Arrange
  fake_operating_system_.SetType(OperatingSystemType::kWindows);
  OperatingSystem::SetForTesting(&fake_operating_system_);

  // Act & Assert
  EXPECT_FALSE(IsMobile());
}

}  // namespace brave_ads
