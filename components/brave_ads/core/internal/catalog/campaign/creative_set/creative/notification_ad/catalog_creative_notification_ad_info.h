/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NOTIFICATION_AD_CATALOG_CREATIVE_NOTIFICATION_AD_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NOTIFICATION_AD_CATALOG_CREATIVE_NOTIFICATION_AD_INFO_H_

#include <vector>

#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/catalog_creative_info.h"
#include "brave/components/brave_ads/core/internal/catalog/campaign/creative_set/creative/notification_ad/catalog_notification_ad_payload_info.h"

namespace brave_ads {

struct CatalogCreativeNotificationAdInfo final : CatalogCreativeInfo {
  bool operator==(const CatalogCreativeNotificationAdInfo&) const = default;

  CatalogNotificationAdPayloadInfo payload;
};

using CatalogCreativeNotificationAdList =
    std::vector<CatalogCreativeNotificationAdInfo>;

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CATALOG_CAMPAIGN_CREATIVE_SET_CREATIVE_NOTIFICATION_AD_CATALOG_CREATIVE_NOTIFICATION_AD_INFO_H_
