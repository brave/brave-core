/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/creative_new_tab_page_ad_unittest_util.h"

#include "bat/ads/internal/bundle/creative_ad_unittest_util.h"
#include "bat/ads/internal/bundle/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/bundle/creative_new_tab_page_ad_wallpaper_focal_point_info.h"
#include "bat/ads/internal/bundle/creative_new_tab_page_ad_wallpaper_info.h"
#include "bat/ads/internal/database/tables/creative_new_tab_page_ads_database_table.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"

namespace ads {

void SaveCreativeAds(const CreativeNewTabPageAdList& creative_ads) {
  database::table::CreativeNewTabPageAds database_table;
  database_table.Save(creative_ads,
                      [](const bool success) { ASSERT_TRUE(success); });
}

CreativeNewTabPageAdList BuildCreativeNewTabPageAds(const int count) {
  CreativeNewTabPageAdList creative_ads;

  for (int i = 0; i < count; i++) {
    const CreativeNewTabPageAdInfo& creative_ad = BuildCreativeNewTabPageAd();
    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

CreativeNewTabPageAdInfo BuildCreativeNewTabPageAd() {
  const CreativeAdInfo& creative_ad = BuildCreativeAd();
  CreativeNewTabPageAdInfo creative_new_tab_page_ad(creative_ad);

  creative_new_tab_page_ad.company_name = "Test Ad Company Name";
  creative_new_tab_page_ad.image_url = "https://brave.com/image";
  creative_new_tab_page_ad.alt = "Test Ad Alt";

  CreativeNewTabPageAdWallpaperInfo wallpaper;
  wallpaper.image_url = "https://brave.com/wallpaper_image";
  CreativeNewTabPageAdWallpaperFocalPointInfo wallpaper_focal_point;
  wallpaper_focal_point.x = 1280;
  wallpaper_focal_point.y = 720;
  wallpaper.focal_point = wallpaper_focal_point;
  creative_new_tab_page_ad.wallpapers.push_back(wallpaper);

  return creative_new_tab_page_ad;
}

}  // namespace ads
