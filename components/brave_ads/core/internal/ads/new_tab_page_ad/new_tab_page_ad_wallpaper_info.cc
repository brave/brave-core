/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ads/new_tab_page_ad_wallpaper_info.h"

#include <tuple>

namespace brave_ads {

bool operator==(const NewTabPageAdWallpaperInfo& lhs,
                const NewTabPageAdWallpaperInfo& rhs) {
  const auto tie = [](const NewTabPageAdWallpaperInfo& wallpaper) {
    return std::tie(wallpaper.focal_point, wallpaper.image_url);
  };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const NewTabPageAdWallpaperInfo& lhs,
                const NewTabPageAdWallpaperInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
