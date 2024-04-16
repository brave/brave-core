/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_CREATIVE_SET_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_CREATIVE_SET_INFO_H_

#include <string>
#include <vector>

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_conversion_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_os_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/catalog_segment_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/inline_content_ad/catalog_creative_inline_content_ad_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/new_tab_page_ad/catalog_creative_new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/notification_ad/catalog_creative_notification_ad_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/promoted_content_ad/catalog_creative_promoted_content_ad_info.h"

namespace brave_ads {

struct CatalogCreativeSetInfo final {
  CatalogCreativeSetInfo();

  CatalogCreativeSetInfo(const CatalogCreativeSetInfo&);
  CatalogCreativeSetInfo& operator=(const CatalogCreativeSetInfo&);

  CatalogCreativeSetInfo(CatalogCreativeSetInfo&&) noexcept;
  CatalogCreativeSetInfo& operator=(CatalogCreativeSetInfo&&) noexcept;

  ~CatalogCreativeSetInfo();

  bool operator==(const CatalogCreativeSetInfo&) const;
  bool operator!=(const CatalogCreativeSetInfo&) const;

  bool DoesSupportOS() const;

  std::string id;
  int per_day = 0;
  int per_week = 0;
  int per_month = 0;
  int total_max = 0;
  double value = 0.0;
  std::string split_test_group;
  CatalogSegmentList segments;
  CatalogOsList oses;
  CatalogConversionList conversions;
  CatalogCreativeNotificationAdList creative_notification_ads;
  CatalogCreativeInlineContentAdList creative_inline_content_ads;
  CatalogCreativeNewTabPageAdList creative_new_tab_page_ads;
  CatalogCreativePromotedContentAdList creative_promoted_content_ads;
};

using CatalogCreativeSetList = std::vector<CatalogCreativeSetInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_CREATIVE_SET_INFO_H_
