/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/command_line_permission_rule.h"

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsCommandLinePermissionRuleTest : public UnitTestBase {
 protected:
  BatAdsCommandLinePermissionRuleTest() = default;

  ~BatAdsCommandLinePermissionRuleTest() override = default;
};

TEST_F(BatAdsCommandLinePermissionRuleTest,
       AllowAdIfDidNotOverrideCommandLineArgsForProduction) {
  // Arrange
  SysInfo().did_override_command_line_args_flag = false;
  MockEnvironment(mojom::Environment::kProduction);

  // Act
  CommandLinePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsCommandLinePermissionRuleTest,
       AllowAdIfDidNotOverrideCommandLineArgsForStaging) {
  // did_override_command_line_args_flag
  SysInfo().did_override_command_line_args_flag = false;
  MockEnvironment(mojom::Environment::kStaging);

  // Act
  CommandLinePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsCommandLinePermissionRuleTest,
       DoNotAllowAdIfDidOverrideCommandLineArgsForProduction) {
  // Arrange
  SysInfo().did_override_command_line_args_flag = true;
  MockEnvironment(mojom::Environment::kProduction);

  // Act
  CommandLinePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsCommandLinePermissionRuleTest,
       AllowAdIfDidOverrideCommandLineArgsForStaging) {
  // Arrange
  SysInfo().did_override_command_line_args_flag = true;
  MockEnvironment(mojom::Environment::kStaging);

  // Act
  CommandLinePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();

  // Assert
  EXPECT_TRUE(is_allowed);
}

}  // namespace ads
