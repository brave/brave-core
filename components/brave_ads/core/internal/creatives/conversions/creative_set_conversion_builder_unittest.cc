/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_builder.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/search_result_ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/units/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeSetConversionBuilderTest : public UnitTestBase {};

TEST_F(BraveAdsCreativeSetConversionBuilderTest, BuildCreativeSetConversion) {
  // Arrange
  const mojom::SearchResultAdInfoPtr search_result_ad =
      test::BuildSearchResultAdWithConversion(
          /*should_use_random_uuids=*/false);

  // Act & Assert
  CreativeSetConversionInfo expected_creative_set_conversion;
  expected_creative_set_conversion.id = kCreativeSetId;
  expected_creative_set_conversion.url_pattern = "https://brave.com/*";
  expected_creative_set_conversion.verifiable_advertiser_public_key_base64 =
      kVerifiableConversionAdvertiserPublicKey;
  expected_creative_set_conversion.observation_window = base::Days(3);
  expected_creative_set_conversion.expire_at =
      Now() + expected_creative_set_conversion.observation_window;
  EXPECT_EQ(expected_creative_set_conversion,
            BuildCreativeSetConversion(search_result_ad));
}

TEST_F(BraveAdsCreativeSetConversionBuilderTest,
       DoNotBuildCreativeSetConversionIfAdDoesNotSupportConversions) {
  // Arrange
  const mojom::SearchResultAdInfoPtr search_result_ad =
      test::BuildSearchResultAd(/*should_use_random_uuids=*/true);

  // Act & Assert
  EXPECT_FALSE(BuildCreativeSetConversion(search_result_ad));
}

}  // namespace brave_ads
