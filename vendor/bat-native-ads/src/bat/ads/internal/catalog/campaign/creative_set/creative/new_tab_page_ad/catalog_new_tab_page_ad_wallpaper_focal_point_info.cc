/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/catalog/campaign/creative_set/creative/new_tab_page_ad/catalog_new_tab_page_ad_wallpaper_focal_point_info.h"

namespace ads {

bool operator==(const CatalogNewTabPageAdWallpaperFocalPointInfo& lhs,
                const CatalogNewTabPageAdWallpaperFocalPointInfo& rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

bool operator!=(const CatalogNewTabPageAdWallpaperFocalPointInfo& lhs,
                const CatalogNewTabPageAdWallpaperFocalPointInfo& rhs) {
  return !(lhs == rhs);
}

}  // namespace ads
