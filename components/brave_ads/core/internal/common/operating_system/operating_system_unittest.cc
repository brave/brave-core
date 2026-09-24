/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system.h"

#include "brave/components/brave_ads/core/internal/common/operating_system/operating_system_types.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsOperatingSystemTest : public test::TestBase {};

TEST_F(BraveAdsOperatingSystemTest, GetNameForAndroid) {
  // Arrange
  fake_operating_system_.SetType(OperatingSystemType::kAndroid);

  // Act & Assert
  EXPECT_EQ("android", OperatingSystem::GetInstance().GetName());
}

TEST_F(BraveAdsOperatingSystemTest, GetNameForIOS) {
  // Arrange
  fake_operating_system_.SetType(OperatingSystemType::kIOS);

  // Act & Assert
  EXPECT_EQ("ios", OperatingSystem::GetInstance().GetName());
}

TEST_F(BraveAdsOperatingSystemTest, GetNameForLinux) {
  // Arrange
  fake_operating_system_.SetType(OperatingSystemType::kLinux);

  // Act & Assert
  EXPECT_EQ("linux", OperatingSystem::GetInstance().GetName());
}

TEST_F(BraveAdsOperatingSystemTest, GetNameForMacOS) {
  // Arrange
  fake_operating_system_.SetType(OperatingSystemType::kMacOS);

  // Act & Assert
  EXPECT_EQ("macos", OperatingSystem::GetInstance().GetName());
}

TEST_F(BraveAdsOperatingSystemTest, GetNameForWindows) {
  // Arrange
  fake_operating_system_.SetType(OperatingSystemType::kWindows);

  // Act & Assert
  EXPECT_EQ("windows", OperatingSystem::GetInstance().GetName());
}

TEST_F(BraveAdsOperatingSystemTest, GetTypeForAndroid) {
  // Arrange
  fake_operating_system_.SetType(OperatingSystemType::kAndroid);

  // Act & Assert
  EXPECT_EQ(OperatingSystemType::kAndroid,
            OperatingSystem::GetInstance().GetType());
}

TEST_F(BraveAdsOperatingSystemTest, GetTypeForIOS) {
  // Arrange
  fake_operating_system_.SetType(OperatingSystemType::kIOS);

  // Act & Assert
  EXPECT_EQ(OperatingSystemType::kIOS,
            OperatingSystem::GetInstance().GetType());
}

TEST_F(BraveAdsOperatingSystemTest, GetTypeForLinux) {
  // Arrange
  fake_operating_system_.SetType(OperatingSystemType::kLinux);

  // Act & Assert
  EXPECT_EQ(OperatingSystemType::kLinux,
            OperatingSystem::GetInstance().GetType());
}

TEST_F(BraveAdsOperatingSystemTest, GetTypeForMacOS) {
  // Arrange
  fake_operating_system_.SetType(OperatingSystemType::kMacOS);

  // Act & Assert
  EXPECT_EQ(OperatingSystemType::kMacOS,
            OperatingSystem::GetInstance().GetType());
}

TEST_F(BraveAdsOperatingSystemTest, GetTypeForWindows) {
  // Arrange
  fake_operating_system_.SetType(OperatingSystemType::kWindows);

  // Act & Assert
  EXPECT_EQ(OperatingSystemType::kWindows,
            OperatingSystem::GetInstance().GetType());
}

}  // namespace brave_ads
