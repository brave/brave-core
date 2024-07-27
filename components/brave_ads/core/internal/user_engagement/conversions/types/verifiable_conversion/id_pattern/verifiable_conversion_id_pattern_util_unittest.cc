/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/id_pattern/verifiable_conversion_id_pattern_util.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_test_constants.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsVerifiableConversionIdPatternUtilTest : public test::TestBase {};

TEST_F(BraveAdsVerifiableConversionIdPatternUtilTest,
       ParseVerifiableUrlRedirectConversionId) {
  // Arrange
  ConversionResourceIdPatternMap resource_id_patterns;
  resource_id_patterns.insert(
      {test::BuildVerifiableConversionUrlPattern(),
       ConversionResourceIdPatternInfo{
           test::BuildVerifiableConversionUrlPattern(),
           ConversionResourceIdPatternSearchInType::kUrlRedirect,
           test::kVerifiableConversionIdPattern}});

  // Act & Assert
  EXPECT_EQ("xyzzy",
            MaybeParseVerifiableConversionId(
                test::BuildVerifiableConversionRedirectChain(),
                test::kVerifiableConversionHtml, resource_id_patterns));
}

TEST_F(BraveAdsVerifiableConversionIdPatternUtilTest,
       ParseVerifiableHtmlConversionId) {
  // Arrange
  ConversionResourceIdPatternMap resource_id_patterns;
  resource_id_patterns.insert(
      {test::kMatchingUrlPattern,
       ConversionResourceIdPatternInfo{
           test::kMatchingUrlPattern,
           ConversionResourceIdPatternSearchInType::kHtml,
           /*id_pattern=*/R"(<div.*id="xyzzy-id".*>(.*)</div>)"}});

  // Act & Assert
  EXPECT_EQ("waldo",
            MaybeParseVerifiableConversionId(
                test::BuildVerifiableConversionRedirectChain(),
                test::kVerifiableConversionHtml, resource_id_patterns));
}

TEST_F(BraveAdsVerifiableConversionIdPatternUtilTest,
       ParseDefaultVerifiableConversionId) {
  // Act & Assert
  EXPECT_EQ("fred", MaybeParseVerifiableConversionId(
                        test::BuildVerifiableConversionRedirectChain(),
                        test::kVerifiableConversionHtml,
                        /*resource_id_patterns=*/{}));
}

TEST_F(BraveAdsVerifiableConversionIdPatternUtilTest,
       DoNotParseVerifiableConversionId) {
  // Act & Assert
  EXPECT_FALSE(MaybeParseVerifiableConversionId(
      test::BuildVerifiableConversionRedirectChain(),
      /*html=*/"", /*resource_id_patterns=*/{}));
}

}  // namespace brave_ads
