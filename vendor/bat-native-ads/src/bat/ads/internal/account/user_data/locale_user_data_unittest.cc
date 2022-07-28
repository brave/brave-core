/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/locale_user_data.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace user_data {

namespace {

std::string GetLocaleAsJson() {
  const base::Value::Dict user_data = GetLocale();

  std::string json;
  base::JSONWriter::Write(user_data, &json);

  return json;
}

}  // namespace

class BatAdsLocaleUserDataTest : public UnitTestBase {
 protected:
  BatAdsLocaleUserDataTest() = default;

  ~BatAdsLocaleUserDataTest() override = default;
};

TEST_F(BatAdsLocaleUserDataTest, GetLocaleForNonReleaseBuildChannel) {
  // Arrange
  MockBuildChannel(BuildChannelType::kNightly);
  MockLocaleHelper(locale_helper_mock_, "en-US");

  // Act
  const std::string json = GetLocaleAsJson();

  // Assert
  const std::string expected_json = "{}";

  EXPECT_EQ(expected_json, json);
}

TEST_F(BatAdsLocaleUserDataTest, GetLocaleForReleaseBuildChannel) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);
  MockLocaleHelper(locale_helper_mock_, "en-US");

  // Act
  const std::string json = GetLocaleAsJson();

  // Assert
  const std::string expected_json = R"({"countryCode":"US"})";

  EXPECT_EQ(expected_json, json);
}

TEST_F(BatAdsLocaleUserDataTest, GetLocaleForCountryNotInAnonymitySet) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);
  MockLocaleHelper(locale_helper_mock_, "en-MC");

  // Act
  const std::string json = GetLocaleAsJson();

  // Assert
  const std::string expected_json = "{}";

  EXPECT_EQ(expected_json, json);
}

TEST_F(BatAdsLocaleUserDataTest,
       GetLocaleForCountryNotInAnonymitySetButShouldClassifyAsOther) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);
  MockLocaleHelper(locale_helper_mock_, "en-CX");

  // Act
  const std::string json = GetLocaleAsJson();

  // Assert
  const std::string expected_json = R"({"countryCode":"??"})";

  EXPECT_EQ(expected_json, json);
}

}  // namespace user_data
}  // namespace ads
