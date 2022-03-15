/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/platform_user_data.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

std::string GetPlatformAsJson() {
  const base::DictionaryValue user_data = user_data::GetPlatform();

  std::string json;
  base::JSONWriter::Write(user_data, &json);

  return json;
}

}  // namespace

class BatAdsPlatformUserDataTest : public UnitTestBase {
 protected:
  BatAdsPlatformUserDataTest() = default;

  ~BatAdsPlatformUserDataTest() override = default;
};

TEST_F(BatAdsPlatformUserDataTest, GetPlatform) {
  // Arrange
  MockPlatformHelper(platform_helper_mock_, PlatformType::kWindows);

  // Act
  const std::string json = GetPlatformAsJson();

  // Assert
  const std::string expected_json = R"({"platform":"windows"})";

  EXPECT_EQ(expected_json, json);
}

}  // namespace ads
