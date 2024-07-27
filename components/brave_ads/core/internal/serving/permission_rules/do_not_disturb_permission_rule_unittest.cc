/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/do_not_disturb_permission_rule.h"

#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsDoNotDisturbPermissionRuleTest : public test::TestBase {};

TEST_F(BraveAdsDoNotDisturbPermissionRuleTest,
       ShouldAllowWhileBrowserIsInactiveBetween6amAnd9pmOnAndroid) {
  // Arrange
  test::MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  NotifyBrowserDidResignActive();
  NotifyBrowserDidEnterBackground();

  AdvanceClockToLocalMidnight();

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
  test::MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterForeground();

  AdvanceClockToLocalMidnight();

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
  test::MockPlatformHelper(platform_helper_mock_, PlatformType::kIOS);

  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterForeground();

  AdvanceClockToLocalMidnight();

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
  test::MockPlatformHelper(platform_helper_mock_, PlatformType::kMacOS);

  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterForeground();

  AdvanceClockToLocalMidnight();

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
  test::MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterForeground();

  AdvanceClockToLocalMidnight();

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
  test::MockPlatformHelper(platform_helper_mock_, PlatformType::kLinux);

  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidEnterForeground();

  AdvanceClockToLocalMidnight();

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
