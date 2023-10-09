/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/dynamic/system_timestamp_user_data.h"

#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsSystemTimestampUserDataTest : public UnitTestBase {};

TEST_F(BraveAdsSystemTimestampUserDataTest,
       BuildSystemTimestampUserDataForRewardsUser) {
  // Arrange
  AdvanceClockTo(
      TimeFromString("November 18 2020 12:34:56.789", /*is_local=*/false));

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "systemTimestamp": "2020-11-18T12:00:00.000Z"
                    })"),
            BuildSystemTimestampUserData());
}

TEST_F(BraveAdsSystemTimestampUserDataTest,
       BuildSystemTimestampUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_TRUE(BuildSystemTimestampUserData().empty());
}

}  // namespace brave_ads
