/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NOTIFICATION_AD_CATALOG_CREATIVE_NOTIFICATION_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NOTIFICATION_AD_CATALOG_CREATIVE_NOTIFICATION_AD_INFO_H_

#include "bat/ads/internal/catalog/campaign/creative_set/creative/catalog_creative_info.h"
#include "bat/ads/internal/catalog/campaign/creative_set/creative/notification_ad/catalog_notification_ad_payload_info.h"

namespace ads {

struct CatalogCreativeNotificationAdInfo final : CatalogCreativeInfo {
  CatalogCreativeNotificationAdInfo();
  CatalogCreativeNotificationAdInfo(
      const CatalogCreativeNotificationAdInfo& info);
  CatalogCreativeNotificationAdInfo& operator=(
      const CatalogCreativeNotificationAdInfo& info);
  ~CatalogCreativeNotificationAdInfo();

  bool operator==(const CatalogCreativeNotificationAdInfo& rhs) const;
  bool operator!=(const CatalogCreativeNotificationAdInfo& rhs) const;

  CatalogNotificationAdPayloadInfo payload;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NOTIFICATION_AD_CATALOG_CREATIVE_NOTIFICATION_AD_INFO_H_
