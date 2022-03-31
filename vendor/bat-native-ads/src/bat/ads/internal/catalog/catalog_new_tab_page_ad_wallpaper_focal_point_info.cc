/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/catalog_new_tab_page_ad_wallpaper_focal_point_info.h"

namespace ads {

CatalogNewTabPageAdWallpaperFocalPointInfo::
    CatalogNewTabPageAdWallpaperFocalPointInfo() = default;

CatalogNewTabPageAdWallpaperFocalPointInfo::
    CatalogNewTabPageAdWallpaperFocalPointInfo(
        const CatalogNewTabPageAdWallpaperFocalPointInfo& info) = default;

CatalogNewTabPageAdWallpaperFocalPointInfo::
    ~CatalogNewTabPageAdWallpaperFocalPointInfo() = default;

bool CatalogNewTabPageAdWallpaperFocalPointInfo::operator==(
    const CatalogNewTabPageAdWallpaperFocalPointInfo& rhs) const {
  return x == rhs.x && y == rhs.y;
}

bool CatalogNewTabPageAdWallpaperFocalPointInfo::operator!=(
    const CatalogNewTabPageAdWallpaperFocalPointInfo& rhs) const {
  return !(*this == rhs);
}

}  // namespace ads
