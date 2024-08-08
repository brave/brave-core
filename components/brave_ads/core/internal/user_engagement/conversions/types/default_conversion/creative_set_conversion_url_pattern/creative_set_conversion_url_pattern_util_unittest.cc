/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/default_conversion/creative_set_conversion_url_pattern/creative_set_conversion_url_pattern_util.h"

#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_test_constants.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeConversionSetUrlPatternUtilTest : public test::TestBase {
};

TEST_F(BraveAdsCreativeConversionSetUrlPatternUtilTest,
       DoesCreativeSetConversionUrlPatternMatchRedirectChain) {
  // Arrange
  const CreativeSetConversionInfo creative_set_conversion =
      test::BuildCreativeSetConversion(test::kCreativeSetId,
                                       test::kMatchingUrlPattern,
                                       /*observation_window=*/base::Days(3));

  // Act & Assert
  EXPECT_TRUE(DoesCreativeSetConversionUrlPatternMatchRedirectChain(
      creative_set_conversion, test::BuildDefaultConversionRedirectChain()));
}

TEST_F(BraveAdsCreativeConversionSetUrlPatternUtilTest,
       DoesCreativeSetConversionUrlPatternNotMatchRedirectChain) {
  // Arrange
  const CreativeSetConversionInfo creative_set_conversion =
      test::BuildCreativeSetConversion(test::kCreativeSetId,
                                       test::kMismatchingUrlPattern,
                                       /*observation_window=*/base::Days(3));

  // Act & Assert
  EXPECT_FALSE(DoesCreativeSetConversionUrlPatternMatchRedirectChain(
      creative_set_conversion, test::BuildDefaultConversionRedirectChain()));
}

}  // namespace brave_ads
