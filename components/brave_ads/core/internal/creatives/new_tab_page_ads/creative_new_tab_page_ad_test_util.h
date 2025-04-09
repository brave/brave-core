/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_AD_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_AD_TEST_UTIL_H_

#include <cstddef>

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type.h"

namespace brave_ads::test {

CreativeNewTabPageAdList BuildCreativeNewTabPageAds(
    CreativeNewTabPageAdWallpaperType wallpaper_type,
    size_t count);
CreativeNewTabPageAdInfo BuildCreativeNewTabPageAd(
    CreativeNewTabPageAdWallpaperType wallpaper_type,
    bool should_generate_random_uuids);

void SaveCreativeNewTabPageAds(const CreativeNewTabPageAdList& creative_ads);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_AD_TEST_UTIL_H_
