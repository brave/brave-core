/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/dynamic/diagnostic_id_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_profile_pref_value.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsDiagnosticIdUserDataTest : public UnitTestBase {};

TEST_F(BraveAdsDiagnosticIdUserDataTest,
       BuildDiagnosticIdUserDataForRewardsUser) {
  // Arrange
  SetProfileStringPrefValue(prefs::kDiagnosticId,
                            "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2");

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "diagnosticId": "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2"
                    })"),
            BuildDiagnosticIdUserData());
}

TEST_F(BraveAdsDiagnosticIdUserDataTest,
       BuildDiagnosticIdUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  SetProfileStringPrefValue(prefs::kDiagnosticId,
                            "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2");

  // Act & Assert
  EXPECT_TRUE(BuildDiagnosticIdUserData().empty());
}

TEST_F(BraveAdsDiagnosticIdUserDataTest,
       DoNotBuildDiagnosticUserDataIfDiagnosticIdIsInvalid) {
  // Arrange
  SetProfileStringPrefValue(prefs::kDiagnosticId, "INVALID");

  // Act & Assert
  EXPECT_TRUE(BuildDiagnosticIdUserData().empty());
}

TEST_F(BraveAdsDiagnosticIdUserDataTest,
       DoNotBuildDiagnosticIdUserDataIfDiagnosticIdIsEmpty) {
  // Arrange
  SetProfileStringPrefValue(prefs::kDiagnosticId, "");

  // Act & Assert
  EXPECT_TRUE(BuildDiagnosticIdUserData().empty());
}

}  // namespace brave_ads
