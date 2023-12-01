/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rule_feature.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsBrowserIsActivePermissionRuleTest : public UnitTestBase {
};

TEST_F(BraveAdsBrowserIsActivePermissionRuleTest, ShouldAllow) {
  // Arrange
  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterForeground();

  // Act & Assert
  EXPECT_TRUE(HasBrowserIsActivePermission());
}

TEST_F(BraveAdsBrowserIsActivePermissionRuleTest, ShouldNotAllow) {
  // Arrange
  NotifyBrowserDidResignActive();
  NotifyBrowserDidEnterBackground();

  // Act & Assert
  EXPECT_FALSE(HasBrowserIsActivePermission());
}

TEST_F(BraveAdsBrowserIsActivePermissionRuleTest,
       ShouldAllowIfPermissionRuleIsDisabled) {
  // Arrange
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeatureWithParameters(
      kPermissionRulesFeature,
      {{"should_only_serve_ads_if_browser_is_active", "false"}});

  NotifyBrowserDidResignActive();
  NotifyBrowserDidEnterBackground();

  // Act & Assert
  EXPECT_TRUE(HasBrowserIsActivePermission());
}

TEST_F(BraveAdsBrowserIsActivePermissionRuleTest,
       ShouldNotAllowIfWindowIsActiveAndBrowserIsBackgrounded) {
  // Arrange
  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterBackground();

  // Act & Assert
  EXPECT_FALSE(HasBrowserIsActivePermission());
}

TEST_F(BraveAdsBrowserIsActivePermissionRuleTest,
       ShouldNotAllowIfWindowIsInactiveAndBrowserIsForegrounded) {
  // Arrange
  NotifyBrowserDidResignActive();
  NotifyBrowserDidEnterForeground();

  // Act & Assert
  EXPECT_FALSE(HasBrowserIsActivePermission());
}

TEST_F(BraveAdsBrowserIsActivePermissionRuleTest,
       ShouldNotAllowIfWindowIsInactiveAndBrowserIsBackgrounded) {
  // Arrange
  NotifyBrowserDidResignActive();
  NotifyBrowserDidEnterBackground();

  // Act & Assert
  EXPECT_FALSE(HasBrowserIsActivePermission());
}

}  // namespace brave_ads
