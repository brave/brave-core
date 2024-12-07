/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_util.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpapers_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ads_database_table.h"

namespace brave_ads::database {

void DeleteCreativeNewTabPageAds() {
  const table::CreativeNewTabPageAds database_table;
  database_table.Delete(base::BindOnce([](bool success) {
    if (!success) {
      BLOG(0, "Failed to delete creative new tab page ads");
    }
  }));
}

void DeleteCreativeNewTabPageAdWallpapers() {
  const table::CreativeNewTabPageAdWallpapers database_table;
  database_table.Delete(base::BindOnce([](bool success) {
    if (!success) {
      BLOG(0, "Failed to delete creative new tab page ad wallpapers");
    }
  }));
}

void SaveCreativeNewTabPageAds(const CreativeNewTabPageAdList& creative_ads) {
  table::CreativeNewTabPageAds database_table;
  database_table.Save(creative_ads, base::BindOnce([](bool success) {
                        if (!success) {
                          return BLOG(
                              0, "Failed to save creative new tab page ads");
                        }

                        BLOG(3, "Successfully saved creative new tab page ads");
                      }));
}

}  // namespace brave_ads::database
