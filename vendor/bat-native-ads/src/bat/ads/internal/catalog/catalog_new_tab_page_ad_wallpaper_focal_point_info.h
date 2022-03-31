/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_NEW_TAB_PAGE_AD_WALLPAPER_FOCAL_POINT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_NEW_TAB_PAGE_AD_WALLPAPER_FOCAL_POINT_INFO_H_

namespace ads {

struct CatalogNewTabPageAdWallpaperFocalPointInfo final {
  CatalogNewTabPageAdWallpaperFocalPointInfo();
  CatalogNewTabPageAdWallpaperFocalPointInfo(
      const CatalogNewTabPageAdWallpaperFocalPointInfo& info);
  ~CatalogNewTabPageAdWallpaperFocalPointInfo();

  bool operator==(const CatalogNewTabPageAdWallpaperFocalPointInfo& rhs) const;
  bool operator!=(const CatalogNewTabPageAdWallpaperFocalPointInfo& rhs) const;

  int x;
  int y;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CATALOG_NEW_TAB_PAGE_AD_WALLPAPER_FOCAL_POINT_INFO_H_
