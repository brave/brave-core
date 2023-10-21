/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/dynamic/confirmation_dynamic_user_data_builder.h"

#include "base/test/mock_callback.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/confirmation_user_data_builder_unittest_util.h"
#include "brave/components/brave_ads/core/internal/account/user_data/build_user_data_callback.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationDynamicUserDataBuilderTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    MockConfirmationUserData();

    AdvanceClockTo(
        TimeFromString("November 18 2020 12:34:56.789", /*is_local=*/false));
  }
};

TEST_F(BraveAdsConfirmationDynamicUserDataBuilderTest,
       BuildConfirmationUserDataForRewardsUser) {
  // Act & Assert
  const base::Value::Dict expected_user_data = base::test::ParseJsonDict(
      R"(
          {
            "diagnosticId": "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2",
            "systemTimestamp": "2020-11-18T12:00:00.000Z"
          })");

  base::MockCallback<BuildUserDataCallback> callback;
  EXPECT_CALL(callback, Run(::testing::Eq(std::ref(expected_user_data))));
  BuildDynamicUserData(callback.Get());
}

TEST_F(BraveAdsConfirmationDynamicUserDataBuilderTest,
       BuildConfirmationUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  base::MockCallback<BuildUserDataCallback> callback;
  EXPECT_CALL(callback, Run(/*user_data=*/::testing::IsEmpty()));
  BuildDynamicUserData(callback.Get());
}

}  // namespace brave_ads
