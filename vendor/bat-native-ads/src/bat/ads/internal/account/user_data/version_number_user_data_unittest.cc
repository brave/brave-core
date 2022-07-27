/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/version_number_user_data.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "brave/components/version_info/version_info.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace user_data {

namespace {

std::string GetVersionNumberAsJson() {
  const base::Value::Dict user_data = GetVersionNumber();

  std::string json;
  base::JSONWriter::Write(user_data, &json);

  return json;
}

}  // namespace

TEST(BatAdsVersionNumberUserDataTest, GetVersionNumber) {
  // Arrange

  // Act
  const std::string json = GetVersionNumberAsJson();

  // Assert
  const std::string expected_version_number =
      version_info::GetBraveChromiumVersionNumber();

  const std::string expected_json = base::StringPrintf(
      R"({"versionNumber":"%s"})", expected_version_number.c_str());

  EXPECT_EQ(expected_json, json);
}

}  // namespace user_data
}  // namespace ads
