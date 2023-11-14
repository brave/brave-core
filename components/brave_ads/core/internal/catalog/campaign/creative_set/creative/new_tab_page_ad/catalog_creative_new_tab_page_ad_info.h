/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NEW_TAB_PAGE_AD_CATALOG_CREATIVE_NEW_TAB_PAGE_AD_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NEW_TAB_PAGE_AD_CATALOG_CREATIVE_NEW_TAB_PAGE_AD_INFO_H_

#include <vector>

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/catalog_creative_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/new_tab_page_ad/catalog_new_tab_page_ad_payload_info.h"

namespace brave_ads {

struct CatalogCreativeNewTabPageAdInfo final : CatalogCreativeInfo {
  bool operator==(const CatalogCreativeNewTabPageAdInfo&) const = default;

  CatalogNewTabPageAdPayloadInfo payload;
};

using CatalogCreativeNewTabPageAdList =
    std::vector<CatalogCreativeNewTabPageAdInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NEW_TAB_PAGE_AD_CATALOG_CREATIVE_NEW_TAB_PAGE_AD_INFO_H_
