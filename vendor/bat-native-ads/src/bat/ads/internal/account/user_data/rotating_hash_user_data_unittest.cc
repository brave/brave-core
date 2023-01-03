/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/rotating_hash_user_data.h"

#include "base/test/values_test_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/sys_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::user_data {

namespace {
constexpr char kCreativeInstanceId[] = "3519f52c-46a4-4c48-9c2b-c264c0067f04";
}  // namespace

class BatAdsRotatingHashUserDataTest : public UnitTestBase {};

TEST_F(BatAdsRotatingHashUserDataTest, GetRotatingHash) {
  // Arrange
  SysInfo().device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  AdvanceClockTo(TimeFromString("2 June 2022 11:00", /*is_local*/ false));

  // Act
  const base::Value::Dict user_data = GetRotatingHash(kCreativeInstanceId);

  // Assert
  const base::Value expected_user_data = base::test::ParseJson(
      R"({"rotating_hash":"06a6D0QCW5onYUDKqCBBXUoil02apd6pcJ47M3Li7hA="})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

TEST_F(BatAdsRotatingHashUserDataTest, RotatingHashMatchesBeforeNextHour) {
  // Arrange
  SysInfo().device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  AdvanceClockTo(TimeFromString("2 June 2022 11:000", /*is_local*/ false));
  const base::Value::Dict user_data_before =
      GetRotatingHash(kCreativeInstanceId);

  // Act
  AdvanceClockBy(base::Hours(1) - base::Seconds(1));

  const base::Value::Dict user_data_after =
      GetRotatingHash(kCreativeInstanceId);

  // Assert
  EXPECT_EQ(user_data_before, user_data_after);
}

TEST_F(BatAdsRotatingHashUserDataTest, RotatingHashDifferentAfterNextHour) {
  // Arrange
  SysInfo().device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  AdvanceClockTo(TimeFromString("2 June 2022 11:00", /*is_local*/ false));
  const base::Value::Dict user_data_before =
      GetRotatingHash(kCreativeInstanceId);

  // Act
  AdvanceClockBy(base::Hours(1));

  const base::Value::Dict user_data_after =
      GetRotatingHash(kCreativeInstanceId);

  // Assert
  EXPECT_NE(user_data_before, user_data_after);
}

TEST_F(BatAdsRotatingHashUserDataTest,
       RotatingHashDifferentForSameHourNextDay) {
  // Arrange
  SysInfo().device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  AdvanceClockTo(TimeFromString("2 June 2022 11:00", /*is_local*/ false));
  const base::Value::Dict user_data_before =
      GetRotatingHash(kCreativeInstanceId);

  // Act
  AdvanceClockBy(base::Days(1));

  const base::Value::Dict user_data_after =
      GetRotatingHash(kCreativeInstanceId);

  // Assert
  EXPECT_NE(user_data_before, user_data_after);
}

}  // namespace ads::user_data
