/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/command_line_permission_rule.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/flags/environment/environment_types.h"
#include "brave/components/brave_ads/core/internal/flags/flag_manager.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace brave_ads {

class BatAdsCommandLinePermissionRuleTest : public UnitTestBase {
 protected:
  CommandLinePermissionRule permission_rule_;
};

TEST_F(BatAdsCommandLinePermissionRuleTest,
       AllowAdIfDidNotOverrideCommandLineSwitchesForProduction) {
  // Arrange
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kProduction);

  FlagManager::GetInstance()->SetDidOverrideFromCommandLineForTesting(false);

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BatAdsCommandLinePermissionRuleTest,
       AllowAdIfDidNotOverrideCommandLineSwitchesForStaging) {
  // Arrange
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kStaging);

  FlagManager::GetInstance()->SetDidOverrideFromCommandLineForTesting(false);

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BatAdsCommandLinePermissionRuleTest,
       DoNotAllowAdIfDidOverrideCommandLineSwitchesForProduction) {
  // Arrange
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kProduction);

  FlagManager::GetInstance()->SetDidOverrideFromCommandLineForTesting(true);

  // Act

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow());
}

TEST_F(BatAdsCommandLinePermissionRuleTest,
       AllowAdIfDidOverrideCommandLineSwitchesForStaging) {
  // Arrange
  FlagManager::GetInstance()->SetEnvironmentTypeForTesting(
      EnvironmentType::kStaging);

  FlagManager::GetInstance()->SetDidOverrideFromCommandLineForTesting(true);

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

}  // namespace brave_ads
