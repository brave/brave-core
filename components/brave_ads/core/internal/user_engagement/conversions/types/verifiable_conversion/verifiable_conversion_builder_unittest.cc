/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_builder.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kHtml[] =
    R"(<html>Hello World!<div id="xyzzy-id">waldo</div><meta name="ad-conversion-id" content="fred"></html>)";

}  // namespace

class BraveAdsVerifiableConversionBuilderTest : public UnitTestBase {};

TEST_F(BraveAdsVerifiableConversionBuilderTest, BuildVerifiableConversionId) {
  // Arrange
  ConversionResourceIdPatternMap resource_id_patterns;
  resource_id_patterns.insert(
      {/*url_pattern=*/"https://foo.com/bar",
       ConversionResourceIdPatternInfo{
           /*url_pattern=*/"https://foo.com/bar",
           /*search_in_type=*/
           ConversionResourceIdPatternSearchInType::kHtml,
           /*id_pattern=*/R"(<div.*id="xyzzy-id".*>(.*)</div>)"}});

  CreativeSetConversionInfo creative_set_conversion;
  creative_set_conversion.verifiable_advertiser_public_key_base64 =
      kVerifiableConversionAdvertiserPublicKey;

  // Act & Assert
  const VerifiableConversionInfo expected_verifiable_conversion{
      "waldo", kVerifiableConversionAdvertiserPublicKey};
  EXPECT_EQ(expected_verifiable_conversion,
            MaybeBuildVerifiableConversion(
                /*redirect_chain=*/{GURL("https://foo.com/bar")}, kHtml,
                resource_id_patterns, creative_set_conversion));
}

TEST_F(BraveAdsVerifiableConversionBuilderTest,
       DoNotBuildVerifiableConversionId) {
  // Arrange
  ConversionResourceIdPatternMap resource_id_patterns;
  resource_id_patterns.insert(
      {/*url_pattern=*/"https://foo.com/bar?qux_id=*",
       ConversionResourceIdPatternInfo{
           /*url_pattern=*/"https://foo.com/bar?qux_id=*",
           /*search_in_type=*/
           ConversionResourceIdPatternSearchInType::kUrlRedirect,
           /*id_pattern=*/"qux_id=(.*)"}});

  CreativeSetConversionInfo creative_set_conversion;

  // Act & Assert
  EXPECT_FALSE(MaybeBuildVerifiableConversion(
      /*redirect_chain=*/{GURL("https://bar.com/foo")}, kHtml,
      resource_id_patterns, creative_set_conversion));
}

}  // namespace brave_ads
