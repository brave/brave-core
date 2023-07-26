/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/confirmations/user_data_builder/dynamic/confirmation_dynamic_user_data_builder.h"

#include "base/functional/bind.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_pref_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConfirmationDynamicUserDataBuilderTest : public UnitTestBase {};

TEST_F(BraveAdsConfirmationDynamicUserDataBuilderTest, Build) {
  // Arrange
  SetDefaultStringPref(prefs::kDiagnosticId,
                       "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2");

  AdvanceClockTo(
      TimeFromString("November 18 2020 12:34:56.789", /*is_local*/ false));

  // Act

  // Assert
  BuildDynamicUserData(base::BindOnce([](base::Value::Dict user_data) {
    EXPECT_EQ(
        base::test::ParseJsonDict(
            R"~({"diagnosticId":"c1298fde-7fdb-401f-a3ce-0b58fe86e6e2","systemTimestamp":"2020-11-18T12:00:00.000Z"})~"),
        user_data);
  }));
}

}  // namespace brave_ads
