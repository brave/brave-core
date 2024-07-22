/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_value_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_constants.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kNewTabPageAdAsJson[] =
    R"(
        {
          "advertiser_id": "5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2",
          "alt": "Test Ad Alt",
          "campaign_id": "84197fc8-830a-4a8e-8339-7a70c2bfa104",
          "company_name": "Test Ad Company Name",
          "creative_instance_id": "546fe7b0-5047-4f28-a11c-81f14edcf0f6",
          "creative_set_id": "c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123",
          "image_url": "https://brave.com/image",
          "placement_id": "9bac9ae4-693c-4569-9b3e-300e357780cf",
          "segment": "untargeted",
          "target_url": "https://brave.com/",
          "type": "new_tab_page_ad",
          "wallpapers": [
            {
              "focal_point": {
                "x": 1280,
                "y": 720
              },
              "image_url": "https://brave.com/wallpaper_image"
            }
          ]
        })";

}  // namespace

class BraveAdsNewTabPageAdValueUtilTest : public test::TestBase {};

TEST_F(BraveAdsNewTabPageAdValueUtilTest, NewTabPageAdFromValue) {
  // Arrange
  const base::Value::Dict dict = base::test::ParseJsonDict(kNewTabPageAdAsJson);

  // Act
  const NewTabPageAdInfo ad = NewTabPageAdFromValue(dict);

  // Assert
  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(/*should_generate_random_uuids=*/false);
  EXPECT_EQ(BuildNewTabPageAd(test::kPlacementId, creative_ad), ad);
}

TEST_F(BraveAdsNewTabPageAdValueUtilTest, NewTabPageAdToValue) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad =
      test::BuildCreativeNewTabPageAd(/*should_generate_random_uuids=*/false);
  const NewTabPageAdInfo ad =
      BuildNewTabPageAd(test::kPlacementId, creative_ad);

  // Act
  const base::Value::Dict dict = NewTabPageAdToValue(ad);

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(kNewTabPageAdAsJson), dict);
}

}  // namespace brave_ads
