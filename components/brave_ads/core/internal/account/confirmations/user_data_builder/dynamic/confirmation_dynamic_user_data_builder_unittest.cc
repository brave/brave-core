/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/dynamic/confirmation_dynamic_user_data_builder.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationDynamicUserDataBuilderTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    test::MockConfirmationUserData();

    AdvanceClockTo(test::TimeFromUTCString("November 18 2020 12:34:56.789"));
  }
};

TEST_F(BraveAdsConfirmationDynamicUserDataBuilderTest, BuildDynamicUserData) {
  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"JSON(
                    {
                      "diagnosticId": "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2",
                      "systemTimestamp": "2020-11-18T12:00:00.000Z"
                    })JSON"),
            BuildDynamicUserData());
}

TEST_F(BraveAdsConfirmationDynamicUserDataBuilderTest,
       DoNotBuildDynamicUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  EXPECT_THAT(BuildDynamicUserData(), ::testing::IsEmpty());
}

}  // namespace brave_ads
