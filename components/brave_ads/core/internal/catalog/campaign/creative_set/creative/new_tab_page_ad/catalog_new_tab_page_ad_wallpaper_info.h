/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NEW_TAB_PAGE_AD_CATALOG_NEW_TAB_PAGE_AD_WALLPAPER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NEW_TAB_PAGE_AD_CATALOG_NEW_TAB_PAGE_AD_WALLPAPER_INFO_H_

#include <vector>

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/new_tab_page_ad/catalog_new_tab_page_ad_wallpaper_focal_point_info.h"
#include "brave/components/brave_ads/core/public/serving/new_tab_page_ad_serving_condition_matcher_util.h"
#include "url/gurl.h"

namespace brave_ads {

struct CatalogNewTabPageAdWallpaperInfo final {
  CatalogNewTabPageAdWallpaperInfo();

  CatalogNewTabPageAdWallpaperInfo(const CatalogNewTabPageAdWallpaperInfo&);
  CatalogNewTabPageAdWallpaperInfo& operator=(
      const CatalogNewTabPageAdWallpaperInfo&);

  CatalogNewTabPageAdWallpaperInfo(CatalogNewTabPageAdWallpaperInfo&&) noexcept;
  CatalogNewTabPageAdWallpaperInfo& operator=(
      CatalogNewTabPageAdWallpaperInfo&&) noexcept;

  ~CatalogNewTabPageAdWallpaperInfo();

  bool operator==(const CatalogNewTabPageAdWallpaperInfo&) const = default;

  GURL image_url;
  CatalogNewTabPageAdWallpaperFocalPointInfo focal_point;
  NewTabPageAdConditionMatcherMap condition_matchers;
};

using CatalogNewTabPageAdWallpaperList =
    std::vector<CatalogNewTabPageAdWallpaperInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NEW_TAB_PAGE_AD_CATALOG_NEW_TAB_PAGE_AD_WALLPAPER_INFO_H_
