/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_builder.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_test_constants.h"

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
           ConversionResourceIdPatternSearchInType::kHtml,
           /*id_pattern=*/R"(<div.*id="xyzzy-id".*>(.*)</div>)"}});

  CreativeSetConversionInfo creative_set_conversion;
  creative_set_conversion.verifiable_advertiser_public_key_base64 =
      test::kVerifiableConversionAdvertiserPublicKeyBase64;

  // Act
  const std::optional<VerifiableConversionInfo> verifiable_conversion =
      MaybeBuildVerifiableConversion(
          /*redirect_chain=*/{GURL("https://foo.com/bar")}, kHtml,
          resource_id_patterns, creative_set_conversion);
  ASSERT_TRUE(verifiable_conversion);

  // Assert
  EXPECT_THAT(
      *verifiable_conversion,
      testing::FieldsAre(/*id*/ "waldo",
                         test::kVerifiableConversionAdvertiserPublicKeyBase64));
}

TEST_F(BraveAdsVerifiableConversionBuilderTest,
       DoNotBuildVerifiableConversionId) {
  // Arrange
  ConversionResourceIdPatternMap resource_id_patterns;
  resource_id_patterns.insert(
      {/*url_pattern=*/"https://foo.com/bar?qux_id=*",
       ConversionResourceIdPatternInfo{
           /*url_pattern=*/"https://foo.com/bar?qux_id=*",
           ConversionResourceIdPatternSearchInType::kUrlRedirect,
           /*id_pattern=*/"qux_id=(.*)"}});

  CreativeSetConversionInfo creative_set_conversion;

  // Act & Assert
  EXPECT_FALSE(MaybeBuildVerifiableConversion(
      /*redirect_chain=*/{GURL("https://bar.com/foo")}, kHtml,
      resource_id_patterns, creative_set_conversion));
}

}  // namespace brave_ads
