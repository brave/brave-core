/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/platform_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsPlatformUserDataTest : public UnitTestBase {};

TEST_F(BraveAdsPlatformUserDataTest, BuildPlatformUserDataForRewardsUser) {
  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "platform": "windows"
                    })"),
            BuildPlatformUserData());
}

TEST_F(BraveAdsPlatformUserDataTest,
       DoNotBuildPlatformUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_TRUE(BuildPlatformUserData().empty());
}

}  // namespace brave_ads
