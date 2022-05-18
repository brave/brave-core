/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/mutated_user_data.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

std::string GetMutatedAsJson() {
  const base::DictionaryValue user_data = user_data::GetMutated();

  std::string json;
  base::JSONWriter::Write(user_data, &json);

  return json;
}

}  // namespace

class BatAdsMutatedUserDataTest : public UnitTestBase {
 protected:
  BatAdsMutatedUserDataTest() = default;

  ~BatAdsMutatedUserDataTest() override = default;
};

TEST_F(BatAdsMutatedUserDataTest, GetMutatedConfirmations) {
  // Arrange
  AdsClientHelper::Get()->SetUint64Pref(
      prefs::kConfirmationsHash,
      /* data/test/confirmations.json has a hash of 3780921521 */ 1251290873);

  // Act
  const std::string json = GetMutatedAsJson();

  // Assert
  const std::string expected_json = R"({"mutated":true})";

  EXPECT_EQ(expected_json, json);
}

TEST_F(BatAdsMutatedUserDataTest, GetMutatedClient) {
  // Arrange
  AdsClientHelper::Get()->SetUint64Pref(
      prefs::kClientHash,
      /* data/test/client.json has a hash of 2810715844 */ 4485170182);

  // Act
  const std::string json = GetMutatedAsJson();

  // Assert
  const std::string expected_json = R"({"mutated":true})";

  EXPECT_EQ(expected_json, json);
}

}  // namespace ads
