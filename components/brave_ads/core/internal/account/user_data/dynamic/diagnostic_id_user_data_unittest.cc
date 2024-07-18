/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/dynamic/diagnostic_id_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/profile_pref_value_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/test_constants.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsDiagnosticIdUserDataTest : public test::TestBase {};

TEST_F(BraveAdsDiagnosticIdUserDataTest,
       BuildDiagnosticIdUserDataForRewardsUser) {
  // Arrange
  test::SetProfileStringPrefValue(prefs::kDiagnosticId, test::kDiagnosticId);

  // Act
  const base::Value::Dict user_data = BuildDiagnosticIdUserData();

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "diagnosticId": "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2"
                    })"),
            user_data);
}

TEST_F(BraveAdsDiagnosticIdUserDataTest,
       BuildDiagnosticIdUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  test::SetProfileStringPrefValue(prefs::kDiagnosticId, test::kDiagnosticId);

  // Act
  const base::Value::Dict user_data = BuildDiagnosticIdUserData();

  // Assert
  EXPECT_THAT(user_data, ::testing::IsEmpty());
}

TEST_F(BraveAdsDiagnosticIdUserDataTest,
       DoNotBuildDiagnosticUserDataIfDiagnosticIdIsInvalid) {
  // Arrange
  test::SetProfileStringPrefValue(prefs::kDiagnosticId, "INVALID");

  // Act
  const base::Value::Dict user_data = BuildDiagnosticIdUserData();

  // Assert
  EXPECT_THAT(user_data, ::testing::IsEmpty());
}

TEST_F(BraveAdsDiagnosticIdUserDataTest,
       DoNotBuildDiagnosticIdUserDataIfDiagnosticIdIsEmpty) {
  // Arrange
  test::SetProfileStringPrefValue(prefs::kDiagnosticId, "");

  // Act
  const base::Value::Dict user_data = BuildDiagnosticIdUserData();

  // Assert
  EXPECT_THAT(user_data, ::testing::IsEmpty());
}

}  // namespace brave_ads
