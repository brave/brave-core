/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_AD_WALLPAPER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_AD_WALLPAPER_INFO_H_

#include <vector>

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_focal_point_info.h"
#include "brave/components/brave_ads/core/public/serving/targeting/condition_matcher/condition_matcher_util.h"
#include "url/gurl.h"

namespace brave_ads {

struct CreativeNewTabPageAdWallpaperInfo final {
  CreativeNewTabPageAdWallpaperInfo();

  CreativeNewTabPageAdWallpaperInfo(const CreativeNewTabPageAdWallpaperInfo&);
  CreativeNewTabPageAdWallpaperInfo& operator=(
      const CreativeNewTabPageAdWallpaperInfo&);

  CreativeNewTabPageAdWallpaperInfo(
      CreativeNewTabPageAdWallpaperInfo&&) noexcept;
  CreativeNewTabPageAdWallpaperInfo& operator=(
      CreativeNewTabPageAdWallpaperInfo&&) noexcept;

  ~CreativeNewTabPageAdWallpaperInfo();

  bool operator==(const CreativeNewTabPageAdWallpaperInfo&) const = default;

  GURL image_url;
  CreativeNewTabPageAdWallpaperFocalPointInfo focal_point;
  ConditionMatcherMap condition_matchers;
};

using CreativeNewTabPageAdWallpaperList =
    std::vector<CreativeNewTabPageAdWallpaperInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_AD_WALLPAPER_INFO_H_
