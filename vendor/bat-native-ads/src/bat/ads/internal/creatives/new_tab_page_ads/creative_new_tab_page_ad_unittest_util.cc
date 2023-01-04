/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_unittest_util.h"

#include "base/check.h"
#include "base/functional/bind.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "bat/ads/internal/creatives/creative_ad_unittest_util.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_focal_point_info.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_info.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"
#include "url/gurl.h"

namespace ads {

void SaveCreativeAds(const CreativeNewTabPageAdList& creative_ads) {
  database::table::CreativeNewTabPageAds database_table;
  database_table.Save(
      creative_ads, base::BindOnce([](const bool success) { CHECK(success); }));
}

CreativeNewTabPageAdList BuildCreativeNewTabPageAds(const int count) {
  CreativeNewTabPageAdList creative_ads;

  for (int i = 0; i < count; i++) {
    const CreativeNewTabPageAdInfo creative_ad =
        BuildCreativeNewTabPageAd(/*should_use_random_guids*/ true);
    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

CreativeNewTabPageAdInfo BuildCreativeNewTabPageAd(
    const bool should_use_random_guids) {
  const CreativeAdInfo creative_ad = BuildCreativeAd(should_use_random_guids);
  CreativeNewTabPageAdInfo creative_new_tab_page_ad(creative_ad);

  creative_new_tab_page_ad.company_name = "Test Ad Company Name";
  creative_new_tab_page_ad.image_url = GURL("https://brave.com/image");
  creative_new_tab_page_ad.alt = "Test Ad Alt";

  CreativeNewTabPageAdWallpaperInfo wallpaper;
  wallpaper.image_url = GURL("https://brave.com/wallpaper_image");
  CreativeNewTabPageAdWallpaperFocalPointInfo wallpaper_focal_point;
  wallpaper_focal_point.x = 1280;
  wallpaper_focal_point.y = 720;
  wallpaper.focal_point = wallpaper_focal_point;
  creative_new_tab_page_ad.wallpapers.push_back(wallpaper);

  return creative_new_tab_page_ad;
}

}  // namespace ads
