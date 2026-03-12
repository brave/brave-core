/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/allocation/ads_allocation_util.h"

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsCreativeAdAllocationUtilTest,
     RandomlyChosenAdIsAlwaysFromTheEligibleList) {
  // Arrange
  const CreativeNewTabPageAdList creative_ads =
      test::BuildCreativeNewTabPageAds(
          CreativeNewTabPageAdWallpaperType::kImage, /*count=*/3);

  // Act
  const CreativeNewTabPageAdInfo chosen_creative_ad =
      ChooseCreativeAdAtRandom(creative_ads);

  // Assert
  EXPECT_TRUE(std::ranges::any_of(
      creative_ads,
      [&chosen_creative_ad](const CreativeNewTabPageAdInfo& creative_ad) {
        return creative_ad.creative_instance_id ==
               chosen_creative_ad.creative_instance_id;
      }));
}

TEST(BraveAdsCreativeAdAllocationUtilTest,
     SingleEligibleAdIsAlwaysTheRandomChoice) {
  // Arrange
  const CreativeNewTabPageAdList creative_ads =
      test::BuildCreativeNewTabPageAds(
          CreativeNewTabPageAdWallpaperType::kImage, /*count=*/1);

  // Act
  const CreativeNewTabPageAdInfo chosen_creative_ad =
      ChooseCreativeAdAtRandom(creative_ads);

  // Assert
  EXPECT_EQ(chosen_creative_ad.creative_instance_id,
            creative_ads.front().creative_instance_id);
}

}  // namespace brave_ads
