/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_util.h"

#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_focal_point_info.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_info.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpapers_database_table.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"

namespace ads {
namespace database {

void DeleteCreativeNewTabPageAds() {
  table::CreativeNewTabPageAds database_table;
  database_table.Delete([](const bool success) {
    if (!success) {
      BLOG(0, "Failed to delete creative new tab page ads");
      return;
    }

    BLOG(3, "Successfully deleted creative new tab page ads");
  });
}

void DeleteCreativeNewTabPageAdWallpapers() {
  table::CreativeNewTabPageAdWallpapers database_table;
  database_table.Delete([](const bool success) {
    if (!success) {
      BLOG(0, "Failed to delete creative new tab page ad wallpapers");
      return;
    }

    BLOG(3, "Successfully deleted creative new tab page ad wallpapers");
  });
}

void SaveCreativeNewTabPageAds(const CreativeNewTabPageAdList& creative_ads) {
  table::CreativeNewTabPageAds database_table;

  database_table.Save(creative_ads, [](const bool success) {
    if (!success) {
      BLOG(0, "Failed to save creative new tab page ads");
      return;
    }

    BLOG(3, "Successfully saved creative new tab page ads");
  });
}

}  // namespace database
}  // namespace ads
