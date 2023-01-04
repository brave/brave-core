/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/new_tab_page_ad_value_util.h"

#include "base/test/values_test_util.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_unittest_util.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/new_tab_page_ad_builder.h"
#include "bat/ads/new_tab_page_ad_info.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

namespace {

constexpr char kPlacementId[] = "f0948316-df6f-4e31-814d-d0b5f2a1f28c";

constexpr char kJson[] =
    R"({"advertiser_id":"5484a63f-eb99-4ba5-a3b0-8c25d3c0e4b2","alt":"Test Ad Alt","campaign_id":"84197fc8-830a-4a8e-8339-7a70c2bfa104","company_name":"Test Ad Company Name","creative_instance_id":"3519f52c-46a4-4c48-9c2b-c264c0067f04","creative_set_id":"c2ba3e7d-f688-4bc4-a053-cbe7ac1e6123","image_url":"https://brave.com/image","placement_id":"f0948316-df6f-4e31-814d-d0b5f2a1f28c","segment":"untargeted","target_url":"https://brave.com/","type":"new_tab_page_ad","wallpapers":[{"focal_point":{"x":1280,"y":720},"image_url":"https://brave.com/wallpaper_image"}]})";

}  // namespace

class BatAdsNewTabPageAdValueUtilTest : public UnitTestBase {};

TEST_F(BatAdsNewTabPageAdValueUtilTest, FromValue) {
  // Arrange
  const base::Value value = base::test::ParseJson(kJson);
  const base::Value::Dict* const dict = value.GetIfDict();
  ASSERT_TRUE(dict);

  // Act
  const NewTabPageAdInfo ad = NewTabPageAdFromValue(*dict);

  // Assert
  const CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAd(/*should_use_random_guids*/ false);
  const NewTabPageAdInfo expected_ad =
      BuildNewTabPageAd(creative_ad, kPlacementId);
  EXPECT_EQ(expected_ad, ad);
}

TEST_F(BatAdsNewTabPageAdValueUtilTest, ToValue) {
  // Arrange
  const CreativeNewTabPageAdInfo creative_ad =
      BuildCreativeNewTabPageAd(/*should_use_random_guids*/ false);
  const NewTabPageAdInfo ad = BuildNewTabPageAd(creative_ad, kPlacementId);

  // Act
  const base::Value::Dict value = NewTabPageAdToValue(ad);

  // Assert
  const base::Value expected_value = base::test::ParseJson(kJson);
  EXPECT_EQ(expected_value, value);
}

}  // namespace ads
