/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/command_line_permission_rule.h"

#include "brave/components/brave_ads/common/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCommandLinePermissionRuleTest : public UnitTestBase {
 protected:
  CommandLinePermissionRule permission_rule_;
};

TEST_F(BraveAdsCommandLinePermissionRuleTest,
       AllowAdIfDidNotOverrideCommandLineSwitchesForProduction) {
  // Arrange
  GlobalState::GetInstance()->Flags().environment_type =
      mojom::EnvironmentType::kProduction;

  GlobalState::GetInstance()->Flags().did_override_from_command_line = false;

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BraveAdsCommandLinePermissionRuleTest,
       AllowAdIfDidNotOverrideCommandLineSwitchesForStaging) {
  // Arrange
  GlobalState::GetInstance()->Flags().environment_type =
      mojom::EnvironmentType::kStaging;

  GlobalState::GetInstance()->Flags().did_override_from_command_line = false;

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BraveAdsCommandLinePermissionRuleTest,
       DoNotAllowAdIfDidOverrideCommandLineSwitchesForProduction) {
  // Arrange
  GlobalState::GetInstance()->Flags().environment_type =
      mojom::EnvironmentType::kProduction;

  GlobalState::GetInstance()->Flags().did_override_from_command_line = true;

  // Act

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow());
}

TEST_F(BraveAdsCommandLinePermissionRuleTest,
       AllowAdIfDidOverrideCommandLineSwitchesForStaging) {
  // Arrange
  GlobalState::GetInstance()->Flags().environment_type =
      mojom::EnvironmentType::kStaging;

  GlobalState::GetInstance()->Flags().did_override_from_command_line = true;

  // Act

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

}  // namespace brave_ads
