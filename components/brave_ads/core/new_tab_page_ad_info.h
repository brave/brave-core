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

namespace brave_ads {

struct ADS_EXPORT NewTabPageAdInfo final : AdInfo {
  NewTabPageAdInfo();

  NewTabPageAdInfo(const NewTabPageAdInfo&);
  NewTabPageAdInfo& operator=(const NewTabPageAdInfo&);

  NewTabPageAdInfo(NewTabPageAdInfo&&) noexcept;
  NewTabPageAdInfo& operator=(NewTabPageAdInfo&&) noexcept;

  ~NewTabPageAdInfo();

  bool operator==(const NewTabPageAdInfo&) const;
  bool operator!=(const NewTabPageAdInfo&) const;

  bool IsValid() const;

  bool HasValidCreativeInstanceIdAndCampaignId() const;

  std::string company_name;
  GURL image_url;
  std::string alt;
  NewTabPageAdWallpaperList wallpapers;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_NEW_TAB_PAGE_AD_INFO_H_
