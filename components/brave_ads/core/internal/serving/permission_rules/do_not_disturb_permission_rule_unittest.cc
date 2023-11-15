/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/permission_rules.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsDoNotDisturbPermissionRuleTest : public UnitTestBase {};

TEST_F(BraveAdsDoNotDisturbPermissionRuleTest,
       ShouldAllowWhileBrowserIsInactiveBetween6amAnd9pmOnAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  NotifyBrowserDidResignActive();
  NotifyBrowserDidEnterBackground();

  AdvanceClockToMidnight(/*is_local=*/true);

  // Act & Assert
  {
    // Verify 5:59 AM
    AdvanceClockBy(base::Hours(5) + base::Minutes(59));
    EXPECT_FALSE(HasDoNotDisturbPermission());
  }

  {
    // Verify 6:00 AM
    AdvanceClockBy(base::Minutes(1));
    EXPECT_TRUE(HasDoNotDisturbPermission());
  }

  {
    // Verify 8:59 PM
    AdvanceClockBy(base::Hours(14) + base::Minutes(59));
    EXPECT_TRUE(HasDoNotDisturbPermission());
  }

  {
    // Verify 9:00 PM
    AdvanceClockBy(base::Minutes(1));
    EXPECT_FALSE(HasDoNotDisturbPermission());
  }
}

TEST_F(BraveAdsDoNotDisturbPermissionRuleTest,
       ShouldAllowWhileBrowserIsActiveOnAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterForeground();

  AdvanceClockToMidnight(/*is_local=*/true);

  // Act & Assert
  {
    // Verify 5:59 AM
    AdvanceClockBy(base::Hours(5) + base::Minutes(59));
    EXPECT_TRUE(HasDoNotDisturbPermission());
  }

  {
    // Verify 6:00 AM
    AdvanceClockBy(base::Minutes(1));
    EXPECT_TRUE(HasDoNotDisturbPermission());
  }

  {
    // Verify 8:59 PM
    AdvanceClockBy(base::Hours(14) + base::Minutes(59));
    EXPECT_TRUE(HasDoNotDisturbPermission());
  }

  {
    // Verify 9:00 PM
    AdvanceClockBy(base::Minutes(1));
    EXPECT_TRUE(HasDoNotDisturbPermission());
  }
}

TEST_F(BraveAdsDoNotDisturbPermissionRuleTest, ShouldAlwaysAllowOnIOS) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kIOS);

  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterForeground();

  AdvanceClockToMidnight(/*is_local=*/true);

  // Act & Assert
  {
    // Verify 00:00 AM
    EXPECT_TRUE(HasDoNotDisturbPermission());
  }

  {
    // Verify 12:00 PM
    AdvanceClockBy(base::Hours(12));
    EXPECT_TRUE(HasDoNotDisturbPermission());
  }
}

TEST_F(BraveAdsDoNotDisturbPermissionRuleTest, ShouldAlwaysAllowOnMacOS) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kMacOS);

  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterForeground();

  AdvanceClockToMidnight(/*is_local=*/true);

  // Act & Assert
  {
    // Verify 00:00 AM
    EXPECT_TRUE(HasDoNotDisturbPermission());
  }

  {
    // Verify 12:00 PM
    AdvanceClockBy(base::Hours(12));
    EXPECT_TRUE(HasDoNotDisturbPermission());
  }
}

TEST_F(BraveAdsDoNotDisturbPermissionRuleTest, ShouldAlwaysAllowOnWindows) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterForeground();

  AdvanceClockToMidnight(/*is_local=*/true);

  // Act & Assert
  {
    // Verify 00:00 AM
    EXPECT_TRUE(HasDoNotDisturbPermission());
  }

  {
    // Verify 12:00 PM
    AdvanceClockBy(base::Hours(12));
    EXPECT_TRUE(HasDoNotDisturbPermission());
  }
}

TEST_F(BraveAdsDoNotDisturbPermissionRuleTest, ShouldAlwaysAllowOnLinux) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kLinux);

  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterForeground();

  AdvanceClockToMidnight(/*is_local=*/true);

  // Act & Assert
  {
    // Verify 00:00 AM
    EXPECT_TRUE(HasDoNotDisturbPermission());
  }

  {
    // Verify 12:00 PM
    AdvanceClockBy(base::Hours(12));
    EXPECT_TRUE(HasDoNotDisturbPermission());
  }
}

}  // namespace brave_ads
