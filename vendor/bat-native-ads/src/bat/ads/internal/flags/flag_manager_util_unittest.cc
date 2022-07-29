/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/flags/flag_manager_util.h"

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/flags/environment/environment_types.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsFlagManagerUtilTest : public UnitTestBase {
 protected:
  BatAdsFlagManagerUtilTest() = default;

  ~BatAdsFlagManagerUtilTest() override = default;
};

TEST_F(BatAdsFlagManagerUtilTest, ShouldDebug) {
  // Arrange

  // Act
  SetShouldDebugForTesting(true);

  // Assert
  EXPECT_TRUE(ShouldDebug());
}

TEST_F(BatAdsFlagManagerUtilTest, ShouldNotDebug) {
  // Arrange

  // Act
  SetShouldDebugForTesting(false);

  // Assert
  EXPECT_FALSE(ShouldDebug());
}

TEST_F(BatAdsFlagManagerUtilTest, DidOverrideVariationsCommandLineSwitches) {
  // Arrange

  // Act
  SetDidOverrideVariationsCommandLineSwitchesForTesting(true);

  // Assert
  EXPECT_TRUE(DidOverrideVariationsCommandLineSwitches());
}

TEST_F(BatAdsFlagManagerUtilTest, DidNotOverrideVariationsCommandLineSwitches) {
  // Arrange

  // Act
  SetDidOverrideVariationsCommandLineSwitchesForTesting(false);

  // Assert
  EXPECT_FALSE(DidOverrideVariationsCommandLineSwitches());
}

TEST_F(BatAdsFlagManagerUtilTest, GetEnvironmentType) {
  // Arrange
  SetEnvironmentTypeForTesting(EnvironmentType::kProduction);

  // Act
  const EnvironmentType environment_type = GetEnvironmentType();

  // Assert
  EXPECT_EQ(EnvironmentType::kProduction, environment_type);
}

TEST_F(BatAdsFlagManagerUtilTest, IsProductionEnvironment) {
  // Arrange

  // Act
  SetEnvironmentTypeForTesting(EnvironmentType::kProduction);

  // Assert
  EXPECT_TRUE(IsProductionEnvironment());
}

TEST_F(BatAdsFlagManagerUtilTest, IsNotProductionEnvironment) {
  // Arrange

  // Act
  SetEnvironmentTypeForTesting(EnvironmentType::kStaging);

  // Assert
  EXPECT_FALSE(IsProductionEnvironment());
}

}  // namespace ads
