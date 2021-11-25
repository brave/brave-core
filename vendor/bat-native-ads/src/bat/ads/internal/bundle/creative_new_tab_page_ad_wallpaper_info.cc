/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/creative_new_tab_page_ad_wallpaper_info.h"

namespace ads {

CreativeNewTabPageAdWallpaperInfo::CreativeNewTabPageAdWallpaperInfo() =
    default;

CreativeNewTabPageAdWallpaperInfo::~CreativeNewTabPageAdWallpaperInfo() =
    default;

bool CreativeNewTabPageAdWallpaperInfo::operator==(
    const CreativeNewTabPageAdWallpaperInfo& rhs) const {
  return image_url == rhs.image_url && focal_point == rhs.focal_point;
}

bool CreativeNewTabPageAdWallpaperInfo::operator!=(
    const CreativeNewTabPageAdWallpaperInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
