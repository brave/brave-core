/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_test_util.h"

#include "base/check_op.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_focal_point_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_info.h"
#include "brave/components/brave_ads/core/internal/segments/segment_test_constants.h"
#include "url/gurl.h"

namespace brave_ads::test {

CreativeNewTabPageAdList BuildCreativeNewTabPageAds(const int count) {
  CHECK_GT(count, 0);

  CreativeNewTabPageAdList creative_ads;

  for (int i = 0; i < count; ++i) {
    CreativeNewTabPageAdInfo creative_ad =
        BuildCreativeNewTabPageAd(/*should_generate_random_uuids=*/true);
    creative_ad.segment = kSegments[i % kSegments.size()];

    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

CreativeNewTabPageAdInfo BuildCreativeNewTabPageAd(
    const bool should_generate_random_uuids) {
  const CreativeAdInfo creative_ad =
      BuildCreativeAd(should_generate_random_uuids);
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

}  // namespace brave_ads::test
