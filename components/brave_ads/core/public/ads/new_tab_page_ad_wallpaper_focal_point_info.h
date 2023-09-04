/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_NEW_TAB_PAGE_AD_WALLPAPER_FOCAL_POINT_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_NEW_TAB_PAGE_AD_WALLPAPER_FOCAL_POINT_INFO_H_

#include "brave/components/brave_ads/core/public/export.h"

namespace brave_ads {

struct ADS_EXPORT NewTabPageAdWallpaperFocalPointInfo final {
  int x = 0;
  int y = 0;
};

bool operator==(const NewTabPageAdWallpaperFocalPointInfo&,
                const NewTabPageAdWallpaperFocalPointInfo&);
bool operator!=(const NewTabPageAdWallpaperFocalPointInfo&,
                const NewTabPageAdWallpaperFocalPointInfo&);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_PUBLIC_ADS_NEW_TAB_PAGE_AD_WALLPAPER_FOCAL_POINT_INFO_H_
