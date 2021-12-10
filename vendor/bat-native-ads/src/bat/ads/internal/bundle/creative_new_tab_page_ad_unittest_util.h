/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_NEW_TAB_PAGE_AD_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_NEW_TAB_PAGE_AD_UNITTEST_UTIL_H_

#include "bat/ads/internal/bundle/creative_new_tab_page_ad_info_aliases.h"

namespace ads {

struct CreativeNewTabPageAdInfo;

void SaveCreativeAds(const CreativeNewTabPageAdList& creative_ads);

CreativeNewTabPageAdList BuildCreativeNewTabPageAds(const int count);
CreativeNewTabPageAdInfo BuildCreativeNewTabPageAd();

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_BUNDLE_CREATIVE_NEW_TAB_PAGE_AD_UNITTEST_UTIL_H_
