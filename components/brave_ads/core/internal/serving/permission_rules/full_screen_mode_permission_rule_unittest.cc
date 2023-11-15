/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rule_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsFullScreenModePermissionRuleTest : public UnitTestBase {
};

TEST_F(BraveAdsFullScreenModePermissionRuleTest, ShouldAllow) {
  // Act & Assert
  EXPECT_TRUE(HasFullScreenModePermission());
}

TEST_F(BraveAdsFullScreenModePermissionRuleTest, ShouldAlwaysAllowOnAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  MockIsBrowserInFullScreenMode(ads_client_mock_, true);

  // Act & Assert
  EXPECT_TRUE(HasFullScreenModePermission());
}

TEST_F(BraveAdsFullScreenModePermissionRuleTest, ShouldAlwaysAllowOnIOS) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kIOS);

  MockIsBrowserInFullScreenMode(ads_client_mock_, true);

  // Act & Assert
  EXPECT_TRUE(HasFullScreenModePermission());
}

TEST_F(BraveAdsFullScreenModePermissionRuleTest, ShouldNotAllow) {
  // Arrange
  MockIsBrowserInFullScreenMode(ads_client_mock_, true);

  // Act & Assert
  EXPECT_FALSE(HasFullScreenModePermission());
}

TEST_F(BraveAdsFullScreenModePermissionRuleTest,
       ShouldAllowIfPermissionRuleIsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kPermissionRulesFeature,
      {{"should_only_serve_ads_in_windowed_mode", "false"}});

  MockIsBrowserInFullScreenMode(ads_client_mock_, true);

  // Act & Assert
  EXPECT_TRUE(HasFullScreenModePermission());
}

}  // namespace brave_ads
