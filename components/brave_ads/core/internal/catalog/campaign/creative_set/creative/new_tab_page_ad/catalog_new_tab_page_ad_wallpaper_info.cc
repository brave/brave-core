/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/new_tab_page_ad/catalog_new_tab_page_ad_wallpaper_info.h"

#include <tuple>

namespace brave_ads {

bool CatalogNewTabPageAdWallpaperInfo::operator==(
    const CatalogNewTabPageAdWallpaperInfo& other) const {
  const auto tie = [](const CatalogNewTabPageAdWallpaperInfo& wallpaper) {
    return std::tie(wallpaper.image_url, wallpaper.focal_point);
  };

  return tie(*this) == tie(other);
}

bool CatalogNewTabPageAdWallpaperInfo::operator!=(
    const CatalogNewTabPageAdWallpaperInfo& other) const {
  return !(*this == other);
}

}  // namespace brave_ads
