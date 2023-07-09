/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_builder.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_builder_util.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsVerifiableConversionBuilderUtilTest : public UnitTestBase {};

TEST_F(BraveAdsVerifiableConversionBuilderUtilTest,
       ShouldExtractVerifiableConversionId) {
  // Arrange
  CreativeSetConversionInfo creative_set_conversion;
  creative_set_conversion.extract_verifiable_id = true;
  creative_set_conversion.verifiable_advertiser_public_key_base64 =
      kVerifiableConversionAdvertiserPublicKey;

  // Act

  // Assert
  EXPECT_TRUE(ShouldExtractVerifiableConversionId(creative_set_conversion));
}

TEST_F(BraveAdsVerifiableConversionBuilderUtilTest,
       ShouldNotExtractVerifiableConversionId) {
  // Arrange
  CreativeSetConversionInfo creative_set_conversion;

  // Act

  // Assert
  EXPECT_FALSE(ShouldExtractVerifiableConversionId(creative_set_conversion));
}

TEST_F(BraveAdsVerifiableConversionBuilderUtilTest, GetVerifiableConversionId) {
  // Arrange
  ConversionResourceIdPatternMap resource_id_patterns;
  resource_id_patterns.insert(
      {/*url_pattern*/ "https://foo.com/*",
       ConversionResourceIdPatternInfo{
           /*url_pattern*/ "https://foo.com/*",
           /*search_in_type*/ ConversionResourceIdPatternSearchInType::kHtml,
           /*id_pattern*/ R"(<div.*id="xyzzy-id".*>(.*)</div>)"}});

  CreativeSetConversionInfo creative_set_conversion;
  creative_set_conversion.extract_verifiable_id = true;
  creative_set_conversion.verifiable_advertiser_public_key_base64 =
      kVerifiableConversionAdvertiserPublicKey;

  // Act

  // Assert
  EXPECT_TRUE(MaybeBuildVerifiableConversion(
      /*redirect_chain*/ {GURL("https://foo.com/bar")},
      /*html*/ R"(<html><div id="xyzzy-id">waldo</div></html>)",
      resource_id_patterns, creative_set_conversion));
}

TEST_F(BraveAdsVerifiableConversionBuilderUtilTest,
       DoNotGetVerifiableConversionId) {
  // Arrange
  ConversionResourceIdPatternMap resource_id_patterns;
  resource_id_patterns.insert(
      {/*url_pattern*/ "https://foo.com/*",
       ConversionResourceIdPatternInfo{
           /*url_pattern*/ "https://foo.com/*",
           /*search_in_type*/ ConversionResourceIdPatternSearchInType::kHtml,
           /*id_pattern*/ R"(<div.*id="xyzzy-id".*>(.*)</div>)"}});

  CreativeSetConversionInfo creative_set_conversion;
  creative_set_conversion.extract_verifiable_id = true;
  creative_set_conversion.verifiable_advertiser_public_key_base64 =
      kVerifiableConversionAdvertiserPublicKey;

  // Act

  // Assert
  EXPECT_FALSE(MaybeBuildVerifiableConversion(
      /*redirect_chain*/ {GURL("https://foo.com/bar")},
      /*html*/ "<html>Hello World!</html>", resource_id_patterns,
      creative_set_conversion));
}

}  // namespace brave_ads
