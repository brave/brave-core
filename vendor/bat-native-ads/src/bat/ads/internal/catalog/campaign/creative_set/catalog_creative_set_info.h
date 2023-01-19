/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_CREATIVE_SET_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_CREATIVE_SET_INFO_H_

#include <string>
#include <vector>

#include "bat/ads/internal/catalog/campaign/creative_set/catalog_os_info.h"
#include "bat/ads/internal/catalog/campaign/creative_set/catalog_segment_info.h"
#include "bat/ads/internal/catalog/campaign/creative_set/creative/inline_content_ad/catalog_creative_inline_content_ad_info.h"
#include "bat/ads/internal/catalog/campaign/creative_set/creative/new_tab_page_ad/catalog_creative_new_tab_page_ad_info.h"
#include "bat/ads/internal/catalog/campaign/creative_set/creative/notification_ad/catalog_creative_notification_ad_info.h"
#include "bat/ads/internal/catalog/campaign/creative_set/creative/promoted_content_ad/catalog_creative_promoted_content_ad_info.h"
#include "bat/ads/internal/conversions/conversion_info.h"

namespace ads {

struct CatalogCreativeSetInfo final {
  CatalogCreativeSetInfo();

  CatalogCreativeSetInfo(const CatalogCreativeSetInfo& other);
  CatalogCreativeSetInfo& operator=(const CatalogCreativeSetInfo& other);

  CatalogCreativeSetInfo(CatalogCreativeSetInfo&& other) noexcept;
  CatalogCreativeSetInfo& operator=(CatalogCreativeSetInfo&& other) noexcept;

  ~CatalogCreativeSetInfo();

  bool operator==(const CatalogCreativeSetInfo& other) const;
  bool operator!=(const CatalogCreativeSetInfo& other) const;

  bool DoesSupportOS() const;

  std::string creative_set_id;
  unsigned int per_day = 0;
  unsigned int per_week = 0;
  unsigned int per_month = 0;
  unsigned int total_max = 0;
  double value = 0.0;
  std::string split_test_group;
  CatalogSegmentList segments;
  CatalogOsList oses;
  CatalogCreativeNotificationAdList creative_notification_ads;
  CatalogCreativeInlineContentAdList creative_inline_content_ads;
  CatalogCreativeNewTabPageAdList creative_new_tab_page_ads;
  CatalogCreativePromotedContentAdList creative_promoted_content_ads;
  ConversionList conversions;
};

using CatalogCreativeSetList = std::vector<CatalogCreativeSetInfo>;

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CATALOG_CREATIVE_SET_INFO_H_
