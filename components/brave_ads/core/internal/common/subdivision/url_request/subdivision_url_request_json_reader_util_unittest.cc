/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/subdivision/url_request/subdivision_url_request_json_reader_util.h"

#include "brave/components/brave_ads/core/internal/common/test/test_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::json::reader {

TEST(BraveAdsSubdivisionUrlRequestJsonReaderUtilTest, ParseJson) {
  // Act & Assert
  EXPECT_EQ("US-CA", ParseSubdivision(
                         R"(
                            {
                              "country": "US",
                              "region": "CA"
                            })"));
}

TEST(BraveAdsSubdivisionUrlRequestJsonReaderUtilTest, DoNotParseMalformedJson) {
  // Act & Assert
  EXPECT_FALSE(ParseSubdivision(test::kMalformedJson));
}

TEST(BraveAdsSubdivisionUrlRequestJsonReaderUtilTest,
     DoNotParseIfMissingCountry) {
  // Act & Assert
  EXPECT_FALSE(ParseSubdivision(
      R"(
          {
            "region": "CA"
          })"));
}

TEST(BraveAdsSubdivisionUrlRequestJsonReaderUtilTest,
     DoNotParseifEmptyCountry) {
  // Act & Assert
  EXPECT_FALSE(ParseSubdivision(
      R"(
          {
            "country": "",
            "region": "CA"
          })"));
}

TEST(BraveAdsSubdivisionUrlRequestJsonReaderUtilTest,
     DoNotParseIfMissingRegion) {
  // Act & Assert
  EXPECT_FALSE(ParseSubdivision(
      R"(
          {
            "country": "US"
          })"));
}

TEST(BraveAdsSubdivisionUrlRequestJsonReaderUtilTest, DoNotParseIfEmptyRegion) {
  // Act & Assert
  EXPECT_FALSE(ParseSubdivision(
      R"(
          {
            "country": "US",
            "region": ""
          })"));
}

}  // namespace brave_ads::json::reader
