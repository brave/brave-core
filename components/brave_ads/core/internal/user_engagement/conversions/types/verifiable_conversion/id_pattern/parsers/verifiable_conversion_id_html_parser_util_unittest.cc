/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/id_pattern/parsers/verifiable_conversion_id_html_parser_util.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/resource/conversion_resource_id_pattern_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/resource/conversion_resource_id_pattern_search_in_types.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kMatchingHtml[] =
    R"(<html><div style="foo" id="xyzzy-id" class="bar">waldo</div></html>)";
constexpr char kMismatchingHtml[] =
    R"(<html><div style="foo" id="qux" class="bar">waldo</div></html>)";
constexpr char kEmptyHtml[] = "";

constexpr char kUrlPattern[] = "https://foo.com/bar";
constexpr ConversionResourceIdPatternSearchInType kSearchInType =
    ConversionResourceIdPatternSearchInType::kHtml;
constexpr char kIdPattern[] = R"(<div.*id="xyzzy-id".*>(.*)</div>)";

}  // namespace

class BraveAdsVerifiableConversionIdHtmlParserUtilTest : public UnitTestBase {};

TEST_F(BraveAdsVerifiableConversionIdHtmlParserUtilTest,
       ParseVerifableConversionIdFromHtml) {
  // Act & Assert
  EXPECT_EQ("waldo",
            MaybeParseVerifableConversionIdFromHtml(
                kMatchingHtml, ConversionResourceIdPatternInfo{
                                   kUrlPattern, kSearchInType, kIdPattern}));
}

TEST_F(BraveAdsVerifiableConversionIdHtmlParserUtilTest,
       DoNotParseMismatchingVerifableConversionIdFromHtml) {
  // Act & Assert
  EXPECT_FALSE(MaybeParseVerifableConversionIdFromHtml(
      kMismatchingHtml,
      ConversionResourceIdPatternInfo{kUrlPattern, kSearchInType, kIdPattern}));
}

TEST_F(BraveAdsVerifiableConversionIdHtmlParserUtilTest,
       DoNotParseVerifableConversionIdFromEmptyHtml) {
  // Act & Assert
  EXPECT_FALSE(MaybeParseVerifableConversionIdFromHtml(
      kEmptyHtml,
      ConversionResourceIdPatternInfo{kUrlPattern, kSearchInType, kIdPattern}));
}

}  // namespace brave_ads
