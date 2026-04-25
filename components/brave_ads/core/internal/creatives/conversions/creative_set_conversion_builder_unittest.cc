/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_builder.h"

#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/creatives/search_result_ads/test/creative_search_result_ad_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsCreativeSetConversionBuilderTest : public test::TestBase {};

TEST_F(BraveAdsCreativeSetConversionBuilderTest,
       FromMojomMaybeBuildCreativeSetConversion) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*use_random_uuids=*/false);

  // Act
  std::optional<CreativeSetConversionInfo> creative_set_conversion =
      FromMojomMaybeBuildCreativeSetConversion(mojom_creative_ad);
  ASSERT_TRUE(creative_set_conversion);

  // Assert
  EXPECT_THAT(
      *creative_set_conversion,
      ::testing::FieldsAre(test::kCreativeSetId,
                           /*url_pattern*/ "https://brave.com/*",
                           /*observation_window*/ base::Days(3),
                           /*expire_at*/ test::Now() + base::Days(3)));
}

TEST_F(BraveAdsCreativeSetConversionBuilderTest,
       DoNotBuildCreativeSetConversionIfUrlPatternIsAbsent) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAdWithConversion(
          /*use_random_uuids=*/false);
  mojom_creative_ad->creative_set_conversion->url_pattern.reset();

  // Act & Assert
  EXPECT_FALSE(FromMojomMaybeBuildCreativeSetConversion(mojom_creative_ad));
}

TEST_F(BraveAdsCreativeSetConversionBuilderTest,
       DoNotBuildCreativeSetConversionIfAdDoesNotSupportConversions) {
  // Arrange
  const mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      test::BuildCreativeSearchResultAd(/*use_random_uuids=*/false);

  // Act & Assert
  EXPECT_FALSE(FromMojomMaybeBuildCreativeSetConversion(mojom_creative_ad));
}

}  // namespace brave_ads
