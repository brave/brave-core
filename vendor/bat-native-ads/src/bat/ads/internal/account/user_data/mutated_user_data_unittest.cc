/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/mutated_user_data.h"

#include "base/test/values_test_util.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace user_data {

class BatAdsMutatedUserDataTest : public UnitTestBase {
 protected:
  BatAdsMutatedUserDataTest() = default;

  ~BatAdsMutatedUserDataTest() override = default;
};

TEST_F(BatAdsMutatedUserDataTest, GetMutatedConfirmations) {
  // Arrange
  AdsClientHelper::GetInstance()->SetUint64Pref(
      prefs::kConfirmationsHash,
      /* data/test/confirmations.json has a hash of 3780921521 */ 1251290873);

  // Act
  const base::Value::Dict user_data = GetMutated();

  // Assert
  const base::Value expected_user_data =
      base::test::ParseJson(R"({"mutated":true})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

TEST_F(BatAdsMutatedUserDataTest, GetMutatedClient) {
  // Arrange
  AdsClientHelper::GetInstance()->SetUint64Pref(
      prefs::kClientHash,
      /* data/test/client.json has a hash of 2810715844 */ 4485170182);

  // Act
  const base::Value::Dict user_data = GetMutated();

  // Assert
  const base::Value expected_user_data =
      base::test::ParseJson(R"({"mutated":true})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

}  // namespace user_data
}  // namespace ads
