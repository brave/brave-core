/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/system_timestamp_user_data.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/time/time.h"
#include "base/values.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

std::string GetSystemTimestampAsJson() {
  const base::DictionaryValue user_data = user_data::GetSystemTimestamp();

  std::string json;
  base::JSONWriter::Write(user_data, &json);

  return json;
}

}  // namespace

class BatAdsSystemTimestampUserDataTest : public UnitTestBase {
 protected:
  BatAdsSystemTimestampUserDataTest() = default;

  ~BatAdsSystemTimestampUserDataTest() override = default;
};

TEST_F(BatAdsSystemTimestampUserDataTest, GetSystemTimestamp) {
  // Arrange
  const base::Time time =
      TimeFromString("November 18 2020 12:34:56.789", /* is_local */ false);
  AdvanceClock(time);

  // Act
  const std::string json = GetSystemTimestampAsJson();

  // Assert
  const std::string expected_json =
      R"({"systemTimestamp":"2020-11-18T12:00:00.000Z"})";

  EXPECT_EQ(expected_json, json);
}

}  // namespace ads
