/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/geographic/subdivision_targeting/subdivision_targeting_json_reader_util.h"

#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::json::reader {

TEST(BraveAdsSubdivisionTargetingJsonReaderUtilTest, ParseValidJson) {
  // Arrange

  // Act

  // Assert
  EXPECT_EQ("US-CA", ParseSubdivisionCode(R"({"country":"US","region":"CA"})"));
}

TEST(BraveAdsSubdivisionTargetingJsonReaderUtilTest, DoNotParseInvalidJson) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(ParseSubdivisionCode("{INVALID}"));
}

TEST(BraveAdsSubdivisionTargetingJsonReaderUtilTest,
     DoNotParseIfMissingCountry) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(ParseSubdivisionCode(R"({"region":"CA"})"));
}

TEST(BraveAdsSubdivisionTargetingJsonReaderUtilTest, DoNotParseifEmptyCountry) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(ParseSubdivisionCode(R"({"country":"","region":"CA"})"));
}

TEST(BraveAdsSubdivisionTargetingJsonReaderUtilTest,
     DoNotParseIfMissingRegion) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(ParseSubdivisionCode(R"({"country":"US"})"));
}

TEST(BraveAdsSubdivisionTargetingJsonReaderUtilTest, DoNotParseIfEmptyRegion) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(ParseSubdivisionCode(R"({"country":"US","region":""})"));
}

}  // namespace brave_ads::json::reader
