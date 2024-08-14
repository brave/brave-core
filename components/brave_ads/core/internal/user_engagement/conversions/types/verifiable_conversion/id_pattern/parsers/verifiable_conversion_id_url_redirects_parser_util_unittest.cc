/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/id_pattern/parsers/verifiable_conversion_id_url_redirects_parser_util.h"

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_test_constants.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/resource/conversion_resource_id_pattern_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/resource/conversion_resource_id_pattern_search_in_types.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsVerifiableConversionIdUrlParserUtilTest,
     ParseVerifableConversionIdFromUrlRedirects) {
  // Act & Assert
  EXPECT_EQ("xyzzy",
            MaybeParseVerifableConversionIdFromUrlRedirects(
                test::BuildVerifiableConversionRedirectChain(),
                ConversionResourceIdPatternInfo{
                    test::kMatchingVerifiableConversionUrlPattern,
                    ConversionResourceIdPatternSearchInType::kUrlRedirect,
                    test::kVerifiableConversionIdPattern}));
}

TEST(BraveAdsVerifiableConversionIdUrlParserUtilTest,
     DoNotParseMismatchingVerifableConversionIdFromUrlRedirects) {
  // Act & Assert
  EXPECT_FALSE(MaybeParseVerifableConversionIdFromUrlRedirects(
      test::BuildVerifiableConversionRedirectChain(),
      ConversionResourceIdPatternInfo{
          test::kMismatchingVerifiableConversionUrlPattern,
          ConversionResourceIdPatternSearchInType::kUrlRedirect,
          test::kVerifiableConversionIdPattern}));
}

}  // namespace brave_ads
