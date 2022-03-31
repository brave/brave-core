/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_NEW_TAB_PAGE_AD_WALLPAPER_FOCAL_POINT_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_NEW_TAB_PAGE_AD_WALLPAPER_FOCAL_POINT_INFO_H_

#include "bat/ads/export.h"

namespace ads {

struct ADS_EXPORT NewTabPageAdWallpaperFocalPointInfo final {
  NewTabPageAdWallpaperFocalPointInfo();
  NewTabPageAdWallpaperFocalPointInfo(
      const NewTabPageAdWallpaperFocalPointInfo& info);
  ~NewTabPageAdWallpaperFocalPointInfo();

  int x;
  int y;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_NEW_TAB_PAGE_AD_WALLPAPER_FOCAL_POINT_INFO_H_
