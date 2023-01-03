/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_AD_INFO_H_

#include <string>
#include <vector>

#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "bat/ads/internal/creatives/new_tab_page_ads/creative_new_tab_page_ad_wallpaper_info.h"
#include "url/gurl.h"

namespace ads {

struct CreativeNewTabPageAdInfo final : CreativeAdInfo {
  CreativeNewTabPageAdInfo();
  explicit CreativeNewTabPageAdInfo(const CreativeAdInfo& creative_ad);

  CreativeNewTabPageAdInfo(const CreativeNewTabPageAdInfo& other);
  CreativeNewTabPageAdInfo& operator=(const CreativeNewTabPageAdInfo& other);

  CreativeNewTabPageAdInfo(CreativeNewTabPageAdInfo&& other) noexcept;
  CreativeNewTabPageAdInfo& operator=(
      CreativeNewTabPageAdInfo&& other) noexcept;

  ~CreativeNewTabPageAdInfo();

  bool operator==(const CreativeNewTabPageAdInfo& other) const;
  bool operator!=(const CreativeNewTabPageAdInfo& other) const;

  std::string company_name;
  GURL image_url;
  std::string alt;
  CreativeNewTabPageAdWallpaperList wallpapers;
};

using CreativeNewTabPageAdList = std::vector<CreativeNewTabPageAdInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CREATIVES_NEW_TAB_PAGE_ADS_CREATIVE_NEW_TAB_PAGE_AD_INFO_H_
