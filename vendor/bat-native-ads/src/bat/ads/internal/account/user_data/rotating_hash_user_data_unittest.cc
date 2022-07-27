/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/rotating_hash_user_data.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/ads.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"
#include "bat/ads/internal/base/unittest/unittest_time_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace user_data {

namespace {

constexpr char kCreativeInstanceId[] = "3519f52c-46a4-4c48-9c2b-c264c0067f04";

std::string GetRotatingHashAsJson() {
  const base::Value::Dict user_data = GetRotatingHash(kCreativeInstanceId);

  std::string json;
  base::JSONWriter::Write(user_data, &json);

  return json;
}

}  // namespace

class BatAdsRotatingHashUserDataTest : public UnitTestBase {
 protected:
  BatAdsRotatingHashUserDataTest() = default;

  ~BatAdsRotatingHashUserDataTest() override = default;
};

TEST_F(BatAdsRotatingHashUserDataTest, GetRotatingHash) {
  // Arrange
  SysInfo().device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  AdvanceClockTo(TimeFromString("2 June 2022 11:00", /* is_local */ false));

  // Act
  const std::string json = GetRotatingHashAsJson();

  // Assert
  const std::string expected_json = R"({"rotating_hash":"1748047652"})";
  EXPECT_EQ(expected_json, json);
}

TEST_F(BatAdsRotatingHashUserDataTest, RotatingHashMatchesBeforeNextHour) {
  // Arrange
  SysInfo().device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  AdvanceClockTo(TimeFromString("2 June 2022 11:000", /* is_local */ false));
  const std::string json_before = GetRotatingHashAsJson();

  // Act
  AdvanceClockBy(base::Hours(1) - base::Seconds(1));
  const std::string json_after = GetRotatingHashAsJson();

  // Assert
  EXPECT_EQ(json_before, json_after);
}

TEST_F(BatAdsRotatingHashUserDataTest, RotatingHashDifferentAfterNextHour) {
  // Arrange
  SysInfo().device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  AdvanceClockTo(TimeFromString("2 June 2022 11:00", /* is_local */ false));
  const std::string json_before = GetRotatingHashAsJson();

  // Act
  AdvanceClockBy(base::Hours(1));
  const std::string json_after = GetRotatingHashAsJson();

  // Assert
  EXPECT_NE(json_before, json_after);
}

TEST_F(BatAdsRotatingHashUserDataTest,
       RotatingHashDifferentForSameHourNextDay) {
  // Arrange
  SysInfo().device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  AdvanceClockTo(TimeFromString("2 June 2022 11:00", /* is_local */ false));
  const std::string json_before = GetRotatingHashAsJson();

  // Act
  AdvanceClockBy(base::Days(1));
  const std::string json_after = GetRotatingHashAsJson();

  // Assert
  EXPECT_NE(json_before, json_after);
}

}  // namespace user_data
}  // namespace ads
