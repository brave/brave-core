/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_CATALOG_CREATIVE_SET_INFO_H_
#define BAT_ADS_INTERNAL_CATALOG_CREATIVE_SET_INFO_H_

#include <string>
#include <vector>

#include "bat/ads/internal/catalog_segment_info.h"
#include "bat/ads/internal/catalog_os_info.h"
#include "bat/ads/internal/catalog_creative_ad_notification_info.h"
#include "bat/ads/internal/catalog_creative_publisher_ad_info.h"
#include "bat/ads/ad_conversion_tracking_info.h"

namespace ads {

struct CatalogCreativeSetInfo {
  CatalogCreativeSetInfo();
  CatalogCreativeSetInfo(
      const CatalogCreativeSetInfo& info);
  ~CatalogCreativeSetInfo();

  std::string creative_set_id;
  unsigned int per_day = 0;
  unsigned int total_max = 0;
  std::vector<CatalogSegmentInfo> segments;
  std::vector<CatalogOsInfo> oses;
  std::vector<CatalogCreativeAdNotificationInfo> ad_notification_creatives;
  std::vector<CatalogCreativePublisherAdInfo> publisher_ad_creatives;
  std::vector<AdConversionTrackingInfo> ad_conversions;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_CATALOG_CREATIVE_SET_INFO_H_
