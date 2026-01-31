/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_util.h"

#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/ad_units/new_tab_page_ad/new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "url/gurl.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

mojom::NewTabPageAdInfoPtr BuildNewTabPageAd() {
  mojom::NewTabPageAdInfoPtr mojom_ad = mojom::NewTabPageAdInfo::New();
  mojom_ad->placement_id = test::kPlacementId;
  mojom_ad->creative_instance_id = test::kCreativeInstanceId;
  mojom_ad->creative_set_id = test::kCreativeSetId;
  mojom_ad->campaign_id = test::kCampaignId;
  mojom_ad->advertiser_id = test::kAdvertiserId;
  mojom_ad->segment = test::kSegment;
  mojom_ad->target_url = GURL(test::kTargetUrl);
  mojom_ad->company_name = test::kTitle;
  mojom_ad->alt = test::kDescription;
  return mojom_ad;
}

}  // namespace

class BraveAdsNewTabPageAdMojomUtilTest : public test::TestBase {};

TEST_F(BraveAdsNewTabPageAdMojomUtilTest, NewTabPageAdFromMojom) {
  // Arrange
  const mojom::NewTabPageAdInfoPtr mojom_ad = BuildNewTabPageAd();

  // Act
  const std::optional<NewTabPageAdInfo> ad = FromMojom(mojom_ad);

  // Assert
  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*should_generate_random_uuids=*/false);
  EXPECT_EQ(BuildNewTabPageAd(test::kPlacementId, creative_ad), ad);
}

TEST_F(BraveAdsNewTabPageAdMojomUtilTest, NewTabPageAdToMojom) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(CreativeNewTabPageAdWallpaperType::kImage,
                                      /*should_generate_random_uuids=*/false);
  const NewTabPageAdInfo ad =
      BuildNewTabPageAd(test::kPlacementId, creative_ad);

  // Act
  const mojom::NewTabPageAdInfoPtr mojom_ad = ToMojom(ad);

  // Assert
  EXPECT_EQ(mojom_ad, BuildNewTabPageAd());
}

TEST_F(BraveAdsNewTabPageAdMojomUtilTest, InvalidNewTabPageAdToMojom) {
  // Arrange
  const NewTabPageAdInfo invalid_ad;

  // Act
  const mojom::NewTabPageAdInfoPtr mojom_ad = ToMojom(invalid_ad);

  // Assert
  EXPECT_FALSE(mojom_ad);
}

}  // namespace brave_ads
