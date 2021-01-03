/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CATALOG_CATALOG_CREATIVE_NEW_TAB_PAGE_AD_INFO_H_
#define BAT_ADS_INTERNAL_CATALOG_CATALOG_CREATIVE_NEW_TAB_PAGE_AD_INFO_H_

#include <vector>

#include "bat/ads/internal/catalog/catalog_creative_info.h"
#include "bat/ads/internal/catalog/catalog_new_tab_page_ad_payload_info.h"

namespace ads {

struct CatalogCreativeNewTabPageAdInfo : CatalogCreativeInfo {
  CatalogCreativeNewTabPageAdInfo();
  CatalogCreativeNewTabPageAdInfo(
      const CatalogCreativeNewTabPageAdInfo& info);
  ~CatalogCreativeNewTabPageAdInfo();

  bool operator==(
      const CatalogCreativeNewTabPageAdInfo& rhs) const;
  bool operator!=(
      const CatalogCreativeNewTabPageAdInfo& rhs) const;

  CatalogNewTabPageAdPayloadInfo payload;
};

using CatalogCreativeNewTabPageAdList =
    std::vector<CatalogCreativeNewTabPageAdInfo>;

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CATALOG_CATALOG_CREATIVE_NEW_TAB_PAGE_AD_INFO_H_
