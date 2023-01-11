/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/diagnostic_id_user_data.h"

#include "base/test/values_test_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/common/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::user_data {

class BatAdsDiagnosticIdUserDataTest : public UnitTestBase {};

TEST_F(BatAdsDiagnosticIdUserDataTest, GetDiagnosticId) {
  // Arrange
  AdsClientHelper::GetInstance()->SetStringPref(
      prefs::kDiagnosticId, "c1298fde-7fdb-401f-a3ce-0b58fe86e6e2");

  // Act
  const base::Value::Dict user_data = GetDiagnosticId();

  // Assert
  const base::Value expected_user_data = base::test::ParseJson(
      R"({"diagnosticId":"c1298fde-7fdb-401f-a3ce-0b58fe86e6e2"})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

TEST_F(BatAdsDiagnosticIdUserDataTest, DoNotGetInvalidDiagnosticId) {
  // Arrange
  AdsClientHelper::GetInstance()->SetStringPref(prefs::kDiagnosticId,
                                                "INVALID");

  // Act
  const base::Value::Dict user_data = GetDiagnosticId();

  // Assert
  EXPECT_TRUE(user_data.empty());
}

TEST_F(BatAdsDiagnosticIdUserDataTest, DoNotGetEmptyDiagnosticId) {
  // Arrange
  AdsClientHelper::GetInstance()->SetStringPref(prefs::kDiagnosticId, "");

  // Act
  const base::Value::Dict user_data = GetDiagnosticId();

  // Assert
  EXPECT_TRUE(user_data.empty());
}

}  // namespace ads::user_data
