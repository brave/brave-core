/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/version_number_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsVersionNumberUserDataTest : public test::TestBase {};

TEST_F(BraveAdsVersionNumberUserDataTest,
       BuildVersionNumberUserDataForRewardsUser) {
  // Act
  const base::Value::Dict user_data = BuildVersionNumberUserData();

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "versionNumber": "1.2.3.4"
                    })"),
            user_data);
}

TEST_F(BraveAdsVersionNumberUserDataTest,
       BuildVersionNumberUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act
  const base::Value::Dict user_data = BuildVersionNumberUserData();

  // Assert
  EXPECT_THAT(user_data, ::testing::IsEmpty());
}

}  // namespace brave_ads
