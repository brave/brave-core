/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/creative_new_tab_page_ad_wallpaper_focal_point_info.h"

namespace ads {

CreativeNewTabPageAdWallpaperFocalPointInfo::
    CreativeNewTabPageAdWallpaperFocalPointInfo() = default;

CreativeNewTabPageAdWallpaperFocalPointInfo::
    ~CreativeNewTabPageAdWallpaperFocalPointInfo() = default;

bool CreativeNewTabPageAdWallpaperFocalPointInfo::operator==(
    const CreativeNewTabPageAdWallpaperFocalPointInfo& rhs) const {
  return x == rhs.x && y == rhs.y;
}

bool CreativeNewTabPageAdWallpaperFocalPointInfo::operator!=(
    const CreativeNewTabPageAdWallpaperFocalPointInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
