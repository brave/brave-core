/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/browser_is_active_permission_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/ads/serving/permission_rules/permission_rule_features.h"
#include "bat/ads/internal/browser/browser_manager.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsBrowserIsActivePermissionRuleTest : public UnitTestBase {};

TEST_F(BatAdsBrowserIsActivePermissionRuleTest, AllowAd) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  BrowserManager::GetInstance()->OnBrowserDidBecomeActive();
  BrowserManager::GetInstance()->OnBrowserDidEnterForeground();

  // Assert
  BrowserIsActivePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsBrowserIsActivePermissionRuleTest, AlwaysAllowAdForAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  // Act
  BrowserManager::GetInstance()->OnBrowserDidResignActive();
  BrowserManager::GetInstance()->OnBrowserDidEnterBackground();

  // Assert
  BrowserIsActivePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsBrowserIsActivePermissionRuleTest, DoNotAllowAd) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  BrowserManager::GetInstance()->OnBrowserDidResignActive();
  BrowserManager::GetInstance()->OnBrowserDidEnterBackground();

  // Assert
  BrowserIsActivePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsBrowserIsActivePermissionRuleTest,
       AllowAdIfPermissionRuleIsDisabled) {
  // Arrange
  base::FieldTrialParams params;
  params["should_only_serve_ads_if_browser_is_active"] = "false";
  std::vector<base::test::FeatureRefAndParams> enabled_features;
  enabled_features.emplace_back(permission_rules::features::kFeature, params);

  const std::vector<base::test::FeatureRef> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  BrowserManager::GetInstance()->OnBrowserDidResignActive();
  BrowserManager::GetInstance()->OnBrowserDidEnterBackground();

  // Assert
  BrowserIsActivePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsBrowserIsActivePermissionRuleTest,
       DoNotAllowAdIfWindowIsActiveAndBrowserIsBackgrounded) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  BrowserManager::GetInstance()->OnBrowserDidBecomeActive();
  BrowserManager::GetInstance()->OnBrowserDidEnterBackground();

  // Assert
  BrowserIsActivePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsBrowserIsActivePermissionRuleTest,
       DoNotAllowAdIfWindowIsInactiveAndBrowserIsForegrounded) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  BrowserManager::GetInstance()->OnBrowserDidResignActive();
  BrowserManager::GetInstance()->OnBrowserDidEnterForeground();

  // Assert
  BrowserIsActivePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
