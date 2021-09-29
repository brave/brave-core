/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/permission_rules/do_not_disturb_frequency_cap.h"

#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_time_util.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsDoNotDisturbFrequencyCapTest : public UnitTestBase {
 protected:
  BatAdsDoNotDisturbFrequencyCapTest() = default;

  ~BatAdsDoNotDisturbFrequencyCapTest() override = default;
};

TEST_F(BatAdsDoNotDisturbFrequencyCapTest,
       AllowAdWhileBrowserIsInactiveBetween6amAnd9pmForAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  BrowserManager::Get()->OnInactive();
  BrowserManager::Get()->OnBackgrounded();

  AdvanceClockToMidnightUTC();

  // Act
  {
    // Verify 5:59 AM
    AdvanceClock(base::TimeDelta::FromHours(5) +
                 base::TimeDelta::FromMinutes(59));

    // Assert
    DoNotDisturbFrequencyCap frequency_cap;
    const bool is_allowed = frequency_cap.ShouldAllow();
    EXPECT_FALSE(is_allowed);
  }

  {
    // Verify 6:00 AM
    AdvanceClock(base::TimeDelta::FromMinutes(1));

    // Assert
    DoNotDisturbFrequencyCap frequency_cap;
    const bool is_allowed = frequency_cap.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 8:59 PM
    AdvanceClock(base::TimeDelta::FromHours(14) +
                 base::TimeDelta::FromMinutes(59));

    // Assert
    DoNotDisturbFrequencyCap frequency_cap;
    const bool is_allowed = frequency_cap.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 9:00 PM
    AdvanceClock(base::TimeDelta::FromMinutes(1));

    // Assert
    DoNotDisturbFrequencyCap frequency_cap;
    const bool is_allowed = frequency_cap.ShouldAllow();
    EXPECT_FALSE(is_allowed);
  }
}

TEST_F(BatAdsDoNotDisturbFrequencyCapTest,
       AllowAdWhileBrowserIsActiveForAndroid) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kAndroid);

  BrowserManager::Get()->OnActive();
  BrowserManager::Get()->OnForegrounded();

  AdvanceClock(Now().LocalMidnight() + base::TimeDelta::FromHours(24) - Now());

  // Act
  {
    // Verify 5:59 AM
    AdvanceClock(base::TimeDelta::FromHours(5) +
                 base::TimeDelta::FromMinutes(59));

    // Assert
    DoNotDisturbFrequencyCap frequency_cap;
    const bool is_allowed = frequency_cap.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 6:00 AM
    AdvanceClock(base::TimeDelta::FromMinutes(1));

    // Assert
    DoNotDisturbFrequencyCap frequency_cap;
    const bool is_allowed = frequency_cap.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 8:59 PM
    AdvanceClock(base::TimeDelta::FromHours(14) +
                 base::TimeDelta::FromMinutes(59));

    // Assert
    DoNotDisturbFrequencyCap frequency_cap;
    const bool is_allowed = frequency_cap.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 9:00 PM
    AdvanceClock(base::TimeDelta::FromMinutes(1));

    // Assert
    DoNotDisturbFrequencyCap frequency_cap;
    const bool is_allowed = frequency_cap.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }
}

TEST_F(BatAdsDoNotDisturbFrequencyCapTest, AlwaysAllowAdForIOS) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kIOS);

  BrowserManager::Get()->OnActive();
  BrowserManager::Get()->OnForegrounded();

  AdvanceClock(Now().LocalMidnight() + base::TimeDelta::FromHours(24) - Now());

  // Act
  {
    // Verify 00:00 AM

    // Assert
    DoNotDisturbFrequencyCap frequency_cap;
    const bool is_allowed = frequency_cap.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 12:00 PM
    AdvanceClock(base::TimeDelta::FromHours(12));

    // Assert
    DoNotDisturbFrequencyCap frequency_cap;
    const bool is_allowed = frequency_cap.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }
}

TEST_F(BatAdsDoNotDisturbFrequencyCapTest, AlwaysAllowAdForMacOS) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  BrowserManager::Get()->OnActive();
  BrowserManager::Get()->OnForegrounded();

  AdvanceClock(Now().LocalMidnight() + base::TimeDelta::FromHours(24) - Now());

  // Act
  {
    // Verify 00:00 AM

    // Assert
    DoNotDisturbFrequencyCap frequency_cap;
    const bool is_allowed = frequency_cap.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 12:00 PM
    AdvanceClock(base::TimeDelta::FromHours(12));

    // Assert
    DoNotDisturbFrequencyCap frequency_cap;
    const bool is_allowed = frequency_cap.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }
}

TEST_F(BatAdsDoNotDisturbFrequencyCapTest, AlwaysAllowAdForWindows) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  BrowserManager::Get()->OnActive();
  BrowserManager::Get()->OnForegrounded();

  AdvanceClock(Now().LocalMidnight() + base::TimeDelta::FromHours(24) - Now());

  // Act
  {
    // Verify 00:00 AM

    // Assert
    DoNotDisturbFrequencyCap frequency_cap;
    const bool is_allowed = frequency_cap.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 12:00 PM
    AdvanceClock(base::TimeDelta::FromHours(12));

    // Assert
    DoNotDisturbFrequencyCap frequency_cap;
    const bool is_allowed = frequency_cap.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }
}

TEST_F(BatAdsDoNotDisturbFrequencyCapTest, AlwaysAllowAdForLinux) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kLinux);

  BrowserManager::Get()->OnActive();
  BrowserManager::Get()->OnForegrounded();

  AdvanceClock(Now().LocalMidnight() + base::TimeDelta::FromHours(24) - Now());

  // Act
  {
    // Verify 00:00 AM

    // Assert
    DoNotDisturbFrequencyCap frequency_cap;
    const bool is_allowed = frequency_cap.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }

  {
    // Verify 12:00 PM
    AdvanceClock(base::TimeDelta::FromHours(12));

    // Assert
    DoNotDisturbFrequencyCap frequency_cap;
    const bool is_allowed = frequency_cap.ShouldAllow();
    EXPECT_TRUE(is_allowed);
  }
}

}  // namespace ads
