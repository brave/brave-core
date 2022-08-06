/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_NEW_TAB_PAGE_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_NEW_TAB_PAGE_AD_INFO_H_

#include <string>

#include "base/values.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/export.h"
#include "bat/ads/new_tab_page_ad_wallpaper_info.h"
#include "url/gurl.h"

namespace ads {

struct ADS_EXPORT NewTabPageAdInfo final : AdInfo {
  NewTabPageAdInfo();
  NewTabPageAdInfo(const NewTabPageAdInfo& info);
  NewTabPageAdInfo& operator=(const NewTabPageAdInfo& info);
  ~NewTabPageAdInfo();

  bool operator==(const NewTabPageAdInfo& rhs) const;

  bool IsValid() const;

  base::Value::Dict ToValue() const;

  std::string ToJson() const;
  bool FromJson(const std::string& json);
  void FromValue(const base::Value::Dict& value);

  std::string company_name;
  GURL image_url;
  std::string alt;
  NewTabPageAdWallpaperList wallpapers;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_NEW_TAB_PAGE_AD_INFO_H_
