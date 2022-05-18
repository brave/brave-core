/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/browser_is_active_permission_rule.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/browser_manager/browser_manager.h"
#include "bat/ads/internal/features/frequency_capping_features.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsBrowserIsActivePermissionRuleTest : public UnitTestBase {
 protected:
  BatAdsBrowserIsActivePermissionRuleTest() = default;

  ~BatAdsBrowserIsActivePermissionRuleTest() override = default;
};

TEST_F(BatAdsBrowserIsActivePermissionRuleTest, AllowAd) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  BrowserManager::Get()->OnDidBecomeActive();
  BrowserManager::Get()->OnDidEnterForeground();

  // Assert
  BrowserIsActivePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsBrowserIsActivePermissionRuleTest, AlwaysAllowAdForAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  // Act
  BrowserManager::Get()->OnDidResignActive();
  BrowserManager::Get()->OnDidEnterBackground();

  // Assert
  BrowserIsActivePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();
  EXPECT_TRUE(is_allowed);
}

TEST_F(BatAdsBrowserIsActivePermissionRuleTest, DoNotAllowAd) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  BrowserManager::Get()->OnDidResignActive();
  BrowserManager::Get()->OnDidEnterBackground();

  // Assert
  BrowserIsActivePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();
  EXPECT_FALSE(is_allowed);
}

TEST_F(BatAdsBrowserIsActivePermissionRuleTest,
       AllowAdIfFrequencyCapIsDisabled) {
  // Arrange
  base::FieldTrialParams kParameters;
  kParameters["should_only_serve_ads_if_browser_is_active"] = "false";
  std::vector<base::test::ScopedFeatureList::FeatureAndParams> enabled_features;
  enabled_features.push_back(
      {features::frequency_capping::kFeature, kParameters});

  const std::vector<base::Feature> disabled_features;

  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitWithFeaturesAndParameters(enabled_features,
                                                    disabled_features);

  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  BrowserManager::Get()->OnDidResignActive();
  BrowserManager::Get()->OnDidEnterBackground();

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
  BrowserManager::Get()->OnDidBecomeActive();
  BrowserManager::Get()->OnDidEnterBackground();

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
  BrowserManager::Get()->OnDidResignActive();
  BrowserManager::Get()->OnDidEnterForeground();

  // Assert
  BrowserIsActivePermissionRule permission_rule;
  const bool is_allowed = permission_rule.ShouldAllow();
  EXPECT_FALSE(is_allowed);
}

}  // namespace ads
