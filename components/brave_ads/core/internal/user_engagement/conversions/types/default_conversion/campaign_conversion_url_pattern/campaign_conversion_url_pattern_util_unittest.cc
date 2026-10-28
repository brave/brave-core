/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/default_conversion/campaign_conversion_url_pattern/campaign_conversion_url_pattern_util.h"

#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/campaign_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/test/campaign_conversion_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/test/conversions_test_constants.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/test/conversions_test_util.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCampaignConversionUrlPatternUtilTest : public test::TestBase {};

TEST_F(BraveAdsCampaignConversionUrlPatternUtilTest,
       DoesCampaignConversionUrlPatternMatchRedirectChain) {
  // Arrange
  const CampaignConversionInfo campaign_conversion =
      test::BuildCampaignConversion(test::kCampaignId,
                                    test::kMatchingUrlPattern,
                                    /*observation_window=*/base::Days(3));

  // Act & Assert
  EXPECT_TRUE(DoesCampaignConversionUrlPatternMatchRedirectChain(
      campaign_conversion, test::BuildDefaultConversionRedirectChain()));
}

TEST_F(BraveAdsCampaignConversionUrlPatternUtilTest,
       DoesEmptyCampaignConversionUrlPatternNotMatchRedirectChain) {
  // Arrange
  const CampaignConversionInfo campaign_conversion =
      test::BuildCampaignConversion(test::kCampaignId,
                                    /*url_pattern=*/"",
                                    /*observation_window=*/base::Days(3));

  // Act & Assert
  EXPECT_FALSE(DoesCampaignConversionUrlPatternMatchRedirectChain(
      campaign_conversion, test::BuildDefaultConversionRedirectChain()));
}

TEST_F(BraveAdsCampaignConversionUrlPatternUtilTest,
       DoesCampaignConversionUrlPatternNotMatchRedirectChain) {
  // Arrange
  const CampaignConversionInfo campaign_conversion =
      test::BuildCampaignConversion(test::kCampaignId,
                                    test::kMismatchingUrlPattern,
                                    /*observation_window=*/base::Days(3));

  // Act & Assert
  EXPECT_FALSE(DoesCampaignConversionUrlPatternMatchRedirectChain(
      campaign_conversion, test::BuildDefaultConversionRedirectChain()));
}

}  // namespace brave_ads
