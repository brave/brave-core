/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_info.h"

#include <tuple>

namespace brave_ads {

bool CreativeNewTabPageAdWallpaperInfo::operator==(
    const CreativeNewTabPageAdWallpaperInfo& other) const {
  const auto tie = [](const CreativeNewTabPageAdWallpaperInfo& wallpaper) {
    return std::tie(wallpaper.image_url, wallpaper.focal_point);
  };

  return tie(*this) == tie(other);
}

bool CreativeNewTabPageAdWallpaperInfo::operator!=(
    const CreativeNewTabPageAdWallpaperInfo& other) const {
  return !(*this == other);
}

}  // namespace brave_ads
