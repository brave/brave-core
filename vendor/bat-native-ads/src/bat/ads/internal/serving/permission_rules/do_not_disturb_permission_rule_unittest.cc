/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/permission_rules/do_not_disturb_permission_rule.h"

#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsDoNotDisturbPermissionRuleTest : public UnitTestBase {
 protected:
  BatAdsDoNotDisturbPermissionRuleTest() = default;

  ~BatAdsDoNotDisturbPermissionRuleTest() override = default;
};

TEST_F(BatAdsDoNotDisturbPermissionRuleTest,
       AllowAdWhileBrowserIsInactiveBetween6amAnd9pmForAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  BrowserManager::Get()->OnDidResignActive();
  BrowserManager::Get()->OnDidEnterBackground();

  AdvanceClockToMidnightUTC();

  // Act
  {
    // Verify 5:59 AM
    AdvanceClock(base::Hours(5) + base::Minutes(59));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_FALSE(is_allowed);
  }

  {
    // Verify 6:00 AM
    AdvanceClock(base::Minutes(1));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 8:59 PM
    AdvanceClock(base::Hours(14) + base::Minutes(59));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 9:00 PM
    AdvanceClock(base::Minutes(1));

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

  BrowserManager::Get()->OnDidBecomeActive();
  BrowserManager::Get()->OnDidEnterForeground();

  AdvanceClock(Now().LocalMidnight() + base::Days(1) - Now());

  // Act
  {
    // Verify 5:59 AM
    AdvanceClock(base::Hours(5) + base::Minutes(59));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 6:00 AM
    AdvanceClock(base::Minutes(1));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 8:59 PM
    AdvanceClock(base::Hours(14) + base::Minutes(59));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 9:00 PM
    AdvanceClock(base::Minutes(1));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }
}

TEST_F(BatAdsDoNotDisturbPermissionRuleTest, AlwaysAllowAdForIOS) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kIOS);

  BrowserManager::Get()->OnDidBecomeActive();
  BrowserManager::Get()->OnDidEnterForeground();

  AdvanceClock(Now().LocalMidnight() + base::Days(1) - Now());

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
    AdvanceClock(base::Hours(12));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }
}

TEST_F(BatAdsDoNotDisturbPermissionRuleTest, AlwaysAllowAdForMacOS) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  BrowserManager::Get()->OnDidBecomeActive();
  BrowserManager::Get()->OnDidEnterForeground();

  AdvanceClock(Now().LocalMidnight() + base::Days(1) - Now());

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
    AdvanceClock(base::Hours(12));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }
}

TEST_F(BatAdsDoNotDisturbPermissionRuleTest, AlwaysAllowAdForWindows) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  BrowserManager::Get()->OnDidBecomeActive();
  BrowserManager::Get()->OnDidEnterForeground();

  AdvanceClock(Now().LocalMidnight() + base::Days(1) - Now());

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
    AdvanceClock(base::Hours(12));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }
}

TEST_F(BatAdsDoNotDisturbPermissionRuleTest, AlwaysAllowAdForLinux) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kLinux);

  BrowserManager::Get()->OnDidBecomeActive();
  BrowserManager::Get()->OnDidEnterForeground();

  AdvanceClock(Now().LocalMidnight() + base::Days(1) - Now());

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
    AdvanceClock(base::Hours(12));

    // Assert
    DoNotDisturbPermissionRule permission_rule;
    const bool is_allowed = permission_rule.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }
}

}  // namespace ads
