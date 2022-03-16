/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/locale_user_data.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

std::string GetLocaleAsJson() {
  const base::DictionaryValue user_data = user_data::GetLocale();

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
  MockLocaleHelper(locale_helper_mock_, "en-US");
  SetBuildChannel(BuildChannelType::kNightly);

  // Act
  const std::string json = GetLocaleAsJson();

  // Assert
  const std::string expected_json = "{}";

  EXPECT_EQ(expected_json, json);
}

TEST_F(BatAdsLocaleUserDataTest, GetLocaleForReleaseBuildChannel) {
  // Arrange
  MockLocaleHelper(locale_helper_mock_, "en-US");
  SetBuildChannel(BuildChannelType::kRelease);

  // Act
  const std::string json = GetLocaleAsJson();

  // Assert
  const std::string expected_json = R"({"countryCode":"US"})";

  EXPECT_EQ(expected_json, json);
}

TEST_F(BatAdsLocaleUserDataTest, GetLocaleForCountryNotInAnonymitySet) {
  // Arrange
  SetBuildChannel(BuildChannelType::kRelease);
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
  SetBuildChannel(BuildChannelType::kRelease);
  MockLocaleHelper(locale_helper_mock_, "en-CX");

  // Act
  const std::string json = GetLocaleAsJson();

  // Assert
  const std::string expected_json = R"({"countryCode":"??"})";

  EXPECT_EQ(expected_json, json);
}

}  // namespace ads
