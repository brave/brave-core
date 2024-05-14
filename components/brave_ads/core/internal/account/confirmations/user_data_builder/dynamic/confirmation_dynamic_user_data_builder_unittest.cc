/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/dynamic/confirmation_dynamic_user_data_builder.h"

#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_converter_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationDynamicUserDataBuilderTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    MockConfirmationUserData();

    AdvanceClockTo(TimeFromUTCString("November 18 2020 12:34:56.789"));
  }
};

TEST_F(BraveAdsConfirmationDynamicUserDataBuilderTest,
       BuildDynamicUserDataForRewardsUser) {
  // Act & Assert
  const base::Value::Dict expected_user_data = base::test::ParseJsonDict(
      R"(
          {
            "diagnosticId": "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2",
            "systemTimestamp": "2020-11-18T12:00:00.000Z"
          })");
  EXPECT_EQ(expected_user_data, BuildDynamicUserData());
}

TEST_F(BraveAdsConfirmationDynamicUserDataBuilderTest,
       BuildDynamicUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act
  const base::Value::Dict user_data = BuildDynamicUserData();

  // Assert
  EXPECT_TRUE(user_data.empty());
}

}  // namespace brave_ads
