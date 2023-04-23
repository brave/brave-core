/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/mutated_user_data.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::user_data {

class BraveAdsMutatedUserDataTest : public UnitTestBase {};

TEST_F(BraveAdsMutatedUserDataTest, GetMutatedConfirmations) {
  // Arrange
  ads_client_mock_.SetUint64Pref(
      prefs::kConfirmationsHash,
      /*data/test/confirmations.json has a hash of 3780921521*/ 1251290873);

  // Act
  const base::Value::Dict user_data = GetMutated();

  // Assert
  const base::Value expected_user_data =
      base::test::ParseJson(R"({"mutated":true})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

TEST_F(BraveAdsMutatedUserDataTest, GetMutatedClient) {
  // Arrange
  ads_client_mock_.SetUint64Pref(
      prefs::kClientHash,
      /*data/test/client.json has a hash of 2810715844*/ 4485170182);

  // Act
  const base::Value::Dict user_data = GetMutated();

  // Assert
  const base::Value expected_user_data =
      base::test::ParseJson(R"({"mutated":true})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

}  // namespace brave_ads::user_data
