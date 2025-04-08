/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_NEW_TAB_PAGE_AD_NEW_TAB_PAGE_AD_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_NEW_TAB_PAGE_AD_NEW_TAB_PAGE_AD_TEST_UTIL_H_

#include "brave/components/brave_ads/core/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_type.h"

namespace brave_ads {

struct NewTabPageAdInfo;

namespace test {

NewTabPageAdInfo BuildNewTabPageAd(
    CreativeNewTabPageAdWallpaperType wallpaper_type,
    bool should_generate_random_uuids);

NewTabPageAdInfo BuildAndSaveNewTabPageAd(
    CreativeNewTabPageAdWallpaperType wallpaper_type,
    bool should_generate_random_uuids);

}  // namespace test

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_NEW_TAB_PAGE_AD_NEW_TAB_PAGE_AD_TEST_UTIL_H_
