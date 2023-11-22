/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/version_number_user_data.h"

#include <string>

#include "base/strings/string_util.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/browser/browser_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsVersionNumberUserDataTest : public UnitTestBase {};

TEST_F(BraveAdsVersionNumberUserDataTest,
       BuildVersionNumberUserDataForRewardsUser) {
  // Act & Assert
  const base::Value::Dict expected_user_data =
      base::test::ParseJsonDict(base::ReplaceStringPlaceholders(
          R"(
              {
                "versionNumber": "$1"
              })",
          {GetBrowserVersionNumber()}, nullptr));
  EXPECT_EQ(expected_user_data, BuildVersionNumberUserData());
}

TEST_F(BraveAdsVersionNumberUserDataTest,
       BuildVersionNumberUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_TRUE(BuildVersionNumberUserData().empty());
}

}  // namespace brave_ads
