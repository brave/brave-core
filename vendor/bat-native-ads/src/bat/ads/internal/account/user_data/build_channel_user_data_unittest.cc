/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/build_channel_user_data.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

std::string GetBuildChannelAsJson() {
  const base::DictionaryValue user_data = user_data::GetBuildChannel();

  std::string json;
  base::JSONWriter::Write(user_data, &json);

  return json;
}

TEST(BatAdsBuildChannelUserDataTest, GetBuildChannel) {
  // Arrange
  SetBuildChannel(BuildChannelType::kRelease);

  // Act
  const std::string json = GetBuildChannelAsJson();

  // Assert
  const std::string expected_json = R"({"buildChannel":"release"})";

  EXPECT_EQ(expected_json, json);
}

}  // namespace ads
