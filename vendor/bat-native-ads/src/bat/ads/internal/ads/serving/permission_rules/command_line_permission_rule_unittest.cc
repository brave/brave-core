/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/command_line_permission_rule.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/flags/environment/environment_types.h"
#include "bat/ads/internal/flags/flag_manager.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCommandLinePermissionRuleTest : public UnitTestBase {};

TEST_F(BatAdsCommandLinePermissionRuleTest,
       AllowAdIfDidNotOverrideCommandLineSwitchesForProduction) {
  // Arrange
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kProduction);

  FlagManager::GetInstance()->SetDidOverrideFromCommandLineForTesting(false);

  // Act
  CommandLinePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsCommandLinePermissionRuleTest,
       AllowAdIfDidNotOverrideCommandLineSwitchesForStaging) {
  // Arrange
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kStaging);

  FlagManager::GetInstance()->SetDidOverrideFromCommandLineForTesting(false);

  // Act
  CommandLinePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsCommandLinePermissionRuleTest,
       DoNotAllowAdIfDidOverrideCommandLineSwitchesForProduction) {
  // Arrange
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kProduction);

  FlagManager::GetInstance()->SetDidOverrideFromCommandLineForTesting(true);

  // Act
  CommandLinePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsCommandLinePermissionRuleTest,
       AllowAdIfDidOverrideCommandLineSwitchesForStaging) {
  // Arrange
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kStaging);

  FlagManager::GetInstance()->SetDidOverrideFromCommandLineForTesting(true);

  // Act
  CommandLinePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

}  // namespace ads
