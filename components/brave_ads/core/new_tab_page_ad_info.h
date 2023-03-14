/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_NEW_TAB_PAGE_AD_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_NEW_TAB_PAGE_AD_INFO_H_

#include <string>

#include "brave/components/brave_ads/core/ad_info.h"
#include "brave/components/brave_ads/core/export.h"
#include "brave/components/brave_ads/core/new_tab_page_ad_wallpaper_info.h"
#include "url/gurl.h"

namespace ads {

struct ADS_EXPORT NewTabPageAdInfo final : AdInfo {
  NewTabPageAdInfo();

  NewTabPageAdInfo(const NewTabPageAdInfo& other);
  NewTabPageAdInfo& operator=(const NewTabPageAdInfo& other);

  NewTabPageAdInfo(NewTabPageAdInfo&& other) noexcept;
  NewTabPageAdInfo& operator=(NewTabPageAdInfo&& other) noexcept;

  ~NewTabPageAdInfo();

  bool operator==(const NewTabPageAdInfo& other) const;
  bool operator!=(const NewTabPageAdInfo& other) const;

  bool IsValid() const;

  std::string company_name;
  GURL image_url;
  std::string alt;
  NewTabPageAdWallpaperList wallpapers;
};

}  // namespace ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_NEW_TAB_PAGE_AD_INFO_H_
