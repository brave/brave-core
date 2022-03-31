/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_NEW_TAB_PAGE_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_NEW_TAB_PAGE_AD_INFO_H_

#include <string>

#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/bundle/creative_new_tab_page_ad_wallpaper_info_aliases.h"

namespace ads {

struct CreativeNewTabPageAdInfo final : CreativeAdInfo {
  CreativeNewTabPageAdInfo();
  CreativeNewTabPageAdInfo(const CreativeNewTabPageAdInfo& info);
  explicit CreativeNewTabPageAdInfo(const CreativeAdInfo& creative_ad);
  ~CreativeNewTabPageAdInfo();

  bool operator==(const CreativeNewTabPageAdInfo& rhs) const;
  bool operator!=(const CreativeNewTabPageAdInfo& rhs) const;

  std::string company_name;
  std::string image_url;
  std::string alt;
  CreativeNewTabPageAdWallpaperList wallpapers;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_NEW_TAB_PAGE_AD_INFO_H_
