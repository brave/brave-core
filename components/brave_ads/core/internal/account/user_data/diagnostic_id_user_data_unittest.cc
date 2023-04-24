/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/diagnostic_id_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsDiagnosticIdUserDataTest : public UnitTestBase {};

TEST_F(BraveAdsDiagnosticIdUserDataTest, BuildDiagnosticIdUserData) {
  // Arrange
  ads_client_mock_.SetStringPref(prefs::kDiagnosticId,
                                 "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2");

  // Act

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"({"diagnosticId":"c1298fde-7fdb-401f-a3ce-0b58fe86e6e2"})"),
            BuildDiagnosticIdUserData());
}

TEST_F(BraveAdsDiagnosticIdUserDataTest,
       DoNotBuildDiagnosticUserDataIfInvalid) {
  // Arrange
  ads_client_mock_.SetStringPref(prefs::kDiagnosticId, "INVALID");

  // Act
  const base::Value::Dict user_data = BuildDiagnosticIdUserData();

  // Assert
  EXPECT_TRUE(user_data.empty());
}

TEST_F(BraveAdsDiagnosticIdUserDataTest,
       DoNotBuildDiagnosticIdUserDataIfEmpty) {
  // Arrange
  ads_client_mock_.SetStringPref(prefs::kDiagnosticId, "");

  // Act
  const base::Value::Dict user_data = BuildDiagnosticIdUserData();

  // Assert
  EXPECT_TRUE(user_data.empty());
}

}  // namespace brave_ads
