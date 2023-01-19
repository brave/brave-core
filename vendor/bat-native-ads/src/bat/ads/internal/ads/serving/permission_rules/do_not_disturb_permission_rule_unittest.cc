/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/serving/permission_rules/do_not_disturb_permission_rule.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsDoNotDisturbPermissionRuleTest : public UnitTestBase {};

TEST_F(BatAdsDoNotDisturbPermissionRuleTest,
       AllowAdWhileBrowserIsInactiveBetween6amAnd9pmForAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  BrowserManager::GetInstance()->OnBrowserDidResignActive();
  BrowserManager::GetInstance()->OnBrowserDidEnterBackground();

  AdvanceClockToMidnight(/*is_local*/ true);

  // Act
  {
    // Verify 5:59 AM
    AdvanceClockBy(base::Hours(5) + base::Minutes(59));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_FALSE(is_allowed);
  }

  {
    // Verify 6:00 AM
    AdvanceClockBy(base::Minutes(1));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 8:59 PM
    AdvanceClockBy(base::Hours(14) + base::Minutes(59));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 9:00 PM
    AdvanceClockBy(base::Minutes(1));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_FALSE(is_allowed);
  }
}

TEST_F(BatAdsDoNotDisturbPermissionRuleTest,
       AllowAdWhileBrowserIsActiveForAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  BrowserManager::GetInstance()->OnBrowserDidBecomeActive();
  BrowserManager::GetInstance()->OnBrowserDidEnterForeground();

  AdvanceClockToMidnight(/*is_local*/ true);

  // Act
  {
    // Verify 5:59 AM
    AdvanceClockBy(base::Hours(5) + base::Minutes(59));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 6:00 AM
    AdvanceClockBy(base::Minutes(1));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 8:59 PM
    AdvanceClockBy(base::Hours(14) + base::Minutes(59));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 9:00 PM
    AdvanceClockBy(base::Minutes(1));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }
}

TEST_F(BatAdsDoNotDisturbPermissionRuleTest, AlwaysAllowAdForIOS) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kIOS);

  BrowserManager::GetInstance()->OnBrowserDidBecomeActive();
  BrowserManager::GetInstance()->OnBrowserDidEnterForeground();

  AdvanceClockToMidnight(/*is_local*/ true);

  // Act
  {
    // Verify 00:00 AM

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 12:00 PM
    AdvanceClockBy(base::Hours(12));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }
}

TEST_F(BatAdsDoNotDisturbPermissionRuleTest, AlwaysAllowAdForMacOS) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  BrowserManager::GetInstance()->OnBrowserDidBecomeActive();
  BrowserManager::GetInstance()->OnBrowserDidEnterForeground();

  AdvanceClockToMidnight(/*is_local*/ true);

  // Act
  {
    // Verify 00:00 AM

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 12:00 PM
    AdvanceClockBy(base::Hours(12));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }
}

TEST_F(BatAdsDoNotDisturbPermissionRuleTest, AlwaysAllowAdForWindows) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  BrowserManager::GetInstance()->OnBrowserDidBecomeActive();
  BrowserManager::GetInstance()->OnBrowserDidEnterForeground();

  AdvanceClockToMidnight(/*is_local*/ true);

  // Act
  {
    // Verify 00:00 AM

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 12:00 PM
    AdvanceClockBy(base::Hours(12));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }
}

TEST_F(BatAdsDoNotDisturbPermissionRuleTest, AlwaysAllowAdForLinux) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kLinux);

  BrowserManager::GetInstance()->OnBrowserDidBecomeActive();
  BrowserManager::GetInstance()->OnBrowserDidEnterForeground();

  AdvanceClockToMidnight(/*is_local*/ true);

  // Act
  {
    // Verify 00:00 AM

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 12:00 PM
    AdvanceClockBy(base::Hours(12));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }
}

}  // namespace ads
