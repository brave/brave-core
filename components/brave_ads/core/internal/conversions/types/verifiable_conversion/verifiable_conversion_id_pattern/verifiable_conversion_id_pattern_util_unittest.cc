/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_id_pattern/verifiable_conversion_id_pattern_util.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kHtml[] =
    R"(<html>Hello World!<div id="xyzzy-id">waldo</div><meta name="ad-conversion-id" content="fred"></html>)";

}  // namespace

class BraveAdsVerifiableConversionIdPatternUtilTest : public UnitTestBase {};

TEST_F(BraveAdsVerifiableConversionIdPatternUtilTest,
       ParseVerifiableUrlRedirectConversionId) {
  // Arrange
  ConversionResourceIdPatternMap resource_id_patterns;
  resource_id_patterns.insert(
      {/*url_pattern*/ "https://foo.com/bar?qux_id=xyz*",
       ConversionResourceIdPatternInfo{
           /*url_pattern*/ "https://foo.com/bar?qux_id=xyz*",
           /*search_in_type*/
           ConversionResourceIdPatternSearchInType::kUrlRedirect,
           /*id_pattern*/ "qux_id=(.*)"}});

  // Act
  const absl::optional<std::string> conversion_id =
      MaybeParseVerifiableConversionId(
          /*redirect_chain*/ {GURL("https://foo.com/bar?qux_id=xyzzy")}, kHtml,
          resource_id_patterns);

  // Assert
  EXPECT_EQ("xyzzy", conversion_id);
}

TEST_F(BraveAdsVerifiableConversionIdPatternUtilTest,
       ParseVerifiableHtmlConversionId) {
  // Arrange
  ConversionResourceIdPatternMap resource_id_patterns;
  resource_id_patterns.insert(
      {/*url_pattern*/ "https://foo.com/*",
       ConversionResourceIdPatternInfo{
           /*url_pattern*/ "https://foo.com/*",
           /*search_in_type*/ ConversionResourceIdPatternSearchInType::kHtml,
           /*id_pattern*/ R"(<div.*id="xyzzy-id".*>(.*)</div>)"}});

  // Act
  const absl::optional<std::string> conversion_id =
      MaybeParseVerifiableConversionId(
          /*redirect_chain*/ {GURL("https://foo.com/bar?qux_id=xyzzy")}, kHtml,
          resource_id_patterns);

  // Assert
  EXPECT_EQ("waldo", conversion_id);
}

TEST_F(BraveAdsVerifiableConversionIdPatternUtilTest,
       ParseDefaultVerifiableConversionId) {
  // Arrange

  // Act
  const absl::optional<std::string> conversion_id =
      MaybeParseVerifiableConversionId(
          /*redirect_chain*/ {GURL("https://foo.com/bar?qux_id=xyzzy")}, kHtml,
          /*resource_id_patterns*/ {});

  // Assert
  EXPECT_EQ("fred", conversion_id);
}

TEST_F(BraveAdsVerifiableConversionIdPatternUtilTest,
       DoNotParseVerifiableConversionId) {
  // Arrange

  // Act

  // Assert
  EXPECT_FALSE(MaybeParseVerifiableConversionId(
      /*redirect_chain*/ {}, /*html*/ {}, /*resource_id_patterns*/ {}));
}

}  // namespace brave_ads
