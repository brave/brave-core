/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NEW_TAB_PAGE_AD_CATALOG_NEW_TAB_PAGE_AD_PAYLOAD_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NEW_TAB_PAGE_AD_CATALOG_NEW_TAB_PAGE_AD_PAYLOAD_INFO_H_

#include <string>

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/new_tab_page_ad/catalog_new_tab_page_ad_wallpaper_info.h"
#include "url/gurl.h"

namespace brave_ads {

struct CatalogNewTabPageAdPayloadInfo final {
  CatalogNewTabPageAdPayloadInfo();

  CatalogNewTabPageAdPayloadInfo(const CatalogNewTabPageAdPayloadInfo&);
  CatalogNewTabPageAdPayloadInfo& operator=(
      const CatalogNewTabPageAdPayloadInfo&);

  CatalogNewTabPageAdPayloadInfo(CatalogNewTabPageAdPayloadInfo&&) noexcept;
  CatalogNewTabPageAdPayloadInfo& operator=(
      CatalogNewTabPageAdPayloadInfo&&) noexcept;

  ~CatalogNewTabPageAdPayloadInfo();

  bool operator==(const CatalogNewTabPageAdPayloadInfo&) const = default;

  std::string company_name;
  GURL image_url;
  std::string alt;
  GURL target_url;
  CatalogNewTabPageAdWallpaperList wallpapers;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NEW_TAB_PAGE_AD_CATALOG_NEW_TAB_PAGE_AD_PAYLOAD_INFO_H_
