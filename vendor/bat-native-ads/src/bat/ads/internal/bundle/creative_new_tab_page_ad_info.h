/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_NEW_TAB_PAGE_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_NEW_TAB_PAGE_AD_INFO_H_

#include <string>
#include <vector>

#include "bat/ads/internal/bundle/creative_ad_info.h"

namespace ads {

struct CreativeNewTabPageAdInfo : CreativeAdInfo {
  CreativeNewTabPageAdInfo();
  ~CreativeNewTabPageAdInfo();

  bool operator==(const CreativeNewTabPageAdInfo& rhs) const;

  bool operator!=(const CreativeNewTabPageAdInfo& rhs) const;

  std::string company_name;
  std::string alt;
};

using CreativeNewTabPageAdList = std::vector<CreativeNewTabPageAdInfo>;

CreativeAdList ToCreativeAdList(
    const CreativeNewTabPageAdList& creative_new_tab_page_ads) {
  CreativeAdList creative_ads;

  for (const auto& creative_new_tab_page_ad : creative_new_tab_page_ads) {
    const CreativeAdInfo creative_ad =
        static_cast<CreativeAdInfo>(creative_new_tab_page_ad);
    creative_ads.push_back(creative_ad);
  }

  return creative_ads;
}

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_NEW_TAB_PAGE_AD_INFO_H_
