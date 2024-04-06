/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/id_pattern/parsers/verifiable_conversion_id_html_meta_tag_parser_util.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsVerifiableConversionIdHtmlMetaTagParserUtilTest
    : public UnitTestBase {};

TEST_F(BraveAdsVerifiableConversionIdHtmlMetaTagParserUtilTest,
       ParseVerifableConversionIdFromHtmlMetaTag) {
  // Act & Assert
  EXPECT_EQ("fred",
            MaybeParseVerifableConversionIdFromHtmlMetaTag(
                /*html=*/R"(<meta name="ad-conversion-id" content="fred">)"));
}

TEST_F(BraveAdsVerifiableConversionIdHtmlMetaTagParserUtilTest,
       DoNotParseMismatchingVerifableConversionIdFromEmptyHtmlMetaTag) {
  // Act & Assert
  EXPECT_FALSE(MaybeParseVerifableConversionIdFromHtmlMetaTag(
      /*html=*/R"(<meta name="foo" content="bar">)"));
}

TEST_F(BraveAdsVerifiableConversionIdHtmlMetaTagParserUtilTest,
       DoNotParseVerifableConversionIdFromEmptyHtmlMetaTag) {
  // Act & Assert
  EXPECT_FALSE(MaybeParseVerifableConversionIdFromHtmlMetaTag(/*html=*/{}));
}

}  // namespace brave_ads
