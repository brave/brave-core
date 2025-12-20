/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_mojom_util.h"

#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

mojom::NewTabPageAdInfoPtr BuildNewTabPageAd() {
  mojom::NewTabPageAdInfoPtr new_tab_page_ad = mojom::NewTabPageAdInfo::New();
  new_tab_page_ad->placement_id = "9bac9ae4-693c-4569-9b3e-300e357780cf";
  new_tab_page_ad->campaign_id = "84197fc8-830a-4a8e-8339-7a70c2bfa104";
  new_tab_page_ad->advertiser_id = "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2";
  new_tab_page_ad->creative_set_id = "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123";
  new_tab_page_ad->creative_instance_id =
      "546fe7b0-5047-4f28-a11c-81f14edcf0f6";
  new_tab_page_ad->company_name = "Test Ad Company Name";
  new_tab_page_ad->segment = "untargeted";
  new_tab_page_ad->alt = "Test Ad Alt";
  new_tab_page_ad->target_url = GURL("https://brave.com/");
  return new_tab_page_ad;
}

}  // namespace

class BraveAdsNewTabPageAdMojomUtilTest : public test::TestBase {};

TEST_F(BraveAdsNewTabPageAdMojomUtilTest, NewTabPageAdFromMojom) {
  // Arrange
  const mojom::NewTabPageAdInfoPtr new_tab_page_ad = BuildNewTabPageAd();

  // Act
  const std::optional<NewTabPageAdInfo> ad =
      NewTabPageAdFromMojom(new_tab_page_ad);

  // Assert
  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*should_generate_random_uuids=*/false);
  EXPECT_EQ(BuildNewTabPageAd(test::kPlacementId, creative_ad), ad);
}

TEST_F(BraveAdsNewTabPageAdMojomUtilTest, NewTabPageAdToValue) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*should_generate_random_uuids=*/false);
  const NewTabPageAdInfo ad =
      BuildNewTabPageAd(test::kPlacementId, creative_ad);

  // Act
  const mojom::NewTabPageAdInfoPtr new_tab_page_ad = NewTabPageAdToMojom(ad);

  // Assert
  EXPECT_EQ(new_tab_page_ad, BuildNewTabPageAd());
}

TEST_F(BraveAdsNewTabPageAdMojomUtilTest, InvalidNewTabPageAdToMojom) {
  // Arrange
  const NewTabPageAdInfo invalid_ad;
  ASSERT_FALSE(invalid_ad.IsValid());

  // Act
  const mojom::NewTabPageAdInfoPtr new_tab_page_ad =
      NewTabPageAdToMojom(invalid_ad);
  EXPECT_FALSE(new_tab_page_ad);
}

}  // namespace brave_ads
