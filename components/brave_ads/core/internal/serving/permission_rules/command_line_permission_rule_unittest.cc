/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCommandLinePermissionRuleTest : public UnitTestBase {
 protected:
};

TEST_F(BraveAdsCommandLinePermissionRuleTest,
       ShouldAllowIfDidNotOverrideCommandLineSwitchesForProduction) {
  // Arrange
  GlobalState::GetInstance()->Flags().environment_type =
      mojom::EnvironmentType::kProduction;

  GlobalState::GetInstance()->Flags().did_override_from_command_line = false;

  // Act & Assert
  EXPECT_TRUE(HasCommandLinePermission());
}

TEST_F(BraveAdsCommandLinePermissionRuleTest,
       ShouldAllowIfDidNotOverrideCommandLineSwitchesForStaging) {
  // Arrange
  GlobalState::GetInstance()->Flags().environment_type =
      mojom::EnvironmentType::kStaging;

  GlobalState::GetInstance()->Flags().did_override_from_command_line = false;

  // Act & Assert
  EXPECT_TRUE(HasCommandLinePermission());
}

TEST_F(BraveAdsCommandLinePermissionRuleTest,
       ShouldNotAllowIfDidOverrideCommandLineSwitchesForProduction) {
  // Arrange
  GlobalState::GetInstance()->Flags().environment_type =
      mojom::EnvironmentType::kProduction;

  GlobalState::GetInstance()->Flags().did_override_from_command_line = true;

  // Act & Assert
  EXPECT_FALSE(HasCommandLinePermission());
}

TEST_F(BraveAdsCommandLinePermissionRuleTest,
       ShouldAllowIfDidOverrideCommandLineSwitchesForStaging) {
  // Arrange
  GlobalState::GetInstance()->Flags().environment_type =
      mojom::EnvironmentType::kStaging;

  GlobalState::GetInstance()->Flags().did_override_from_command_line = true;

  // Act & Assert
  EXPECT_TRUE(HasCommandLinePermission());
}

}  // namespace brave_ads
