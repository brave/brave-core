/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_NEW_TAB_PAGE_AD_WALLPAPER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_NEW_TAB_PAGE_AD_WALLPAPER_INFO_H_

#include <vector>

#include "brave/components/brave_ads/core/export.h"
#include "brave/components/brave_ads/core/new_tab_page_ad_wallpaper_focal_point_info.h"
#include "url/gurl.h"

namespace ads {

struct ADS_EXPORT NewTabPageAdWallpaperInfo final {
  GURL image_url;
  NewTabPageAdWallpaperFocalPointInfo focal_point;
};

bool operator==(const NewTabPageAdWallpaperInfo& lhs,
                const NewTabPageAdWallpaperInfo& rhs);
bool operator!=(const NewTabPageAdWallpaperInfo& lhs,
                const NewTabPageAdWallpaperInfo& rhs);

using NewTabPageAdWallpaperList = std::vector<NewTabPageAdWallpaperInfo>;

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_NEW_TAB_PAGE_AD_WALLPAPER_INFO_H_
