/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_focal_point_info.h"

namespace ads {

bool operator==(const CreativeNewTabPageAdWallpaperFocalPointInfo& lhs,
                const CreativeNewTabPageAdWallpaperFocalPointInfo& rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

bool operator!=(const CreativeNewTabPageAdWallpaperFocalPointInfo& lhs,
                const CreativeNewTabPageAdWallpaperFocalPointInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace ads
