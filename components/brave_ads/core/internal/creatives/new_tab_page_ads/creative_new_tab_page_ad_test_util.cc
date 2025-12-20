/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_test_util.h"

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_test_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/segments/segment_test_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_ads::test {

CreativeNewTabPageAdList BuildCreativeNewTabPageAds(
    CreativeNewTabPageAdWallpaperType wallpaper_type,
    size_t count) {
  CHECK_GT(count, 0U);

  CreativeNewTabPageAdList creative_ads;

  for (size_t i = 0; i < count; ++i) {
    CreativeNewTabPageAdInfo creative_ad = BuildCreativeNewTabPageAd(
        wallpaper_type, /*should_generate_random_uuids=*/true);
    creative_ad.segment = kSegments[i % kSegments.size()];

    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

CreativeNewTabPageAdInfo BuildCreativeNewTabPageAd(
    CreativeNewTabPageAdWallpaperType wallpaper_type,
    bool should_generate_random_uuids) {
  const CreativeAdInfo creative_ad =
      BuildCreativeAd(should_generate_random_uuids);
  CreativeNewTabPageAdInfo creative_new_tab_page_ad(creative_ad);

  creative_new_tab_page_ad.wallpaper_type = wallpaper_type;
  creative_new_tab_page_ad.company_name = "Test Ad Title";
  creative_new_tab_page_ad.alt = "Test Ad Description";

  return creative_new_tab_page_ad;
}

void SaveCreativeNewTabPageAds(const CreativeNewTabPageAdList& creative_ads) {
  database::table::CreativeNewTabPageAds database_table;
  database_table.Save(
      creative_ads, base::BindOnce([](bool success) { ASSERT_TRUE(success); }));
}

}  // namespace brave_ads::test
