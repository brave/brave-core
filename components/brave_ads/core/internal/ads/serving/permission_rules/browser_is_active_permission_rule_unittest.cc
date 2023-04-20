/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/browser_is_active_permission_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/ads/serving/permission_rules/permission_rule_features.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsBrowserIsActivePermissionRuleTest : public UnitTestBase {
 protected:
  BrowserIsActivePermissionRule permission_rule_;
};

TEST_F(BraveAdsBrowserIsActivePermissionRuleTest, AllowAd) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterForeground();

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BraveAdsBrowserIsActivePermissionRuleTest, AlwaysAllowAdForAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  // Act
  NotifyBrowserDidResignActive();
  NotifyBrowserDidEnterBackground();

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BraveAdsBrowserIsActivePermissionRuleTest, DoNotAllowAd) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  NotifyBrowserDidResignActive();
  NotifyBrowserDidEnterBackground();

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow());
}

TEST_F(BraveAdsBrowserIsActivePermissionRuleTest,
       AllowAdIfPermissionRuleIsDisabled) {
  // Arrange
  base::FieldTrialParams params;
  params["should_only_serve_ads_if_browser_is_active"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(kPermissionRulesFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  NotifyBrowserDidResignActive();
  NotifyBrowserDidEnterBackground();

  // Assert
  EXPECT_TRUE(permission_rule_.ShouldAllow());
}

TEST_F(BraveAdsBrowserIsActivePermissionRuleTest,
       DoNotAllowAdIfWindowIsActiveAndBrowserIsBackgrounded) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterBackground();

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow());
}

TEST_F(BraveAdsBrowserIsActivePermissionRuleTest,
       DoNotAllowAdIfWindowIsInactiveAndBrowserIsForegrounded) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  NotifyBrowserDidResignActive();
  NotifyBrowserDidEnterForeground();

  // Assert
  EXPECT_FALSE(permission_rule_.ShouldAllow());
}

}  // namespace brave_ads
