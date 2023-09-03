/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ads/new_tab_page_ad_wallpaper_focal_point_info.h"

#include <tuple>

namespace brave_ads {

bool operator==(const NewTabPageAdWallpaperFocalPointInfo& lhs,
                const NewTabPageAdWallpaperFocalPointInfo& rhs) {
  const auto tie =
      [](const NewTabPageAdWallpaperFocalPointInfo& wallpaper_focal_point) {
        return std::tie(wallpaper_focal_point.x, wallpaper_focal_point.y);
      };

  return tie(lhs) == tie(rhs);
}

bool operator!=(const NewTabPageAdWallpaperFocalPointInfo& lhs,
                const NewTabPageAdWallpaperFocalPointInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace brave_ads
