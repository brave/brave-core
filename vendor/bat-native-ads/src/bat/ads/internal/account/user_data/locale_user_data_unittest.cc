/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/locale_user_data.h"

#include "base/test/values_test_util.h"
#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {
namespace user_data {

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
  const base::Value::Dict user_data = GetLocale();

  // Assert
  const base::Value expected_user_data = base::test::ParseJson("{}");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

TEST_F(BatAdsLocaleUserDataTest, GetLocaleForReleaseBuildChannel) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);
  MockLocaleHelper(locale_helper_mock_, "en-US");

  // Act
  const base::Value::Dict user_data = GetLocale();

  // Assert
  const base::Value expected_user_data =
      base::test::ParseJson(R"({"countryCode":"US"})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

TEST_F(BatAdsLocaleUserDataTest, GetLocaleForCountryNotInAnonymitySet) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);
  MockLocaleHelper(locale_helper_mock_, "en-MC");

  // Act
  const base::Value::Dict user_data = GetLocale();

  // Assert
  const base::Value expected_user_data = base::test::ParseJson("{}");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

TEST_F(BatAdsLocaleUserDataTest,
       GetLocaleForCountryNotInAnonymitySetButShouldClassifyAsOther) {
  // Arrange
  MockBuildChannel(BuildChannelType::kRelease);
  MockLocaleHelper(locale_helper_mock_, "en-CX");

  // Act
  const base::Value::Dict user_data = GetLocale();

  // Assert
  const base::Value expected_user_data =
      base::test::ParseJson(R"({"countryCode":"??"})");
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

}  // namespace user_data
}  // namespace ads
