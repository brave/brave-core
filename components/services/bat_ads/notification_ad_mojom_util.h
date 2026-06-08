/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BAT_ADS_NOTIFICATION_AD_MOJOM_UTIL_H_
#define BRAVE_COMPONENTS_SERVICES_BAT_ADS_NOTIFICATION_AD_MOJOM_UTIL_H_

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"

namespace bat_ads {

inline brave_ads::mojom::NotificationAdInfoPtr NotificationAdToMojom(
    const brave_ads::NotificationAdInfo& ad) {
  auto mojom_notification_ad = brave_ads::mojom::NotificationAdInfo::New();
  mojom_notification_ad->type = ad.type;
  mojom_notification_ad->placement_id = ad.placement_id;
  mojom_notification_ad->creative_instance_id = ad.creative_instance_id;
  mojom_notification_ad->creative_set_id = ad.creative_set_id;
  mojom_notification_ad->campaign_id = ad.campaign_id;
  mojom_notification_ad->advertiser_id = ad.advertiser_id;
  mojom_notification_ad->segment = ad.segment;
  mojom_notification_ad->title = ad.title;
  mojom_notification_ad->body = ad.body;
  mojom_notification_ad->target_url = ad.target_url;
  return mojom_notification_ad;
}

}  // namespace bat_ads

#endif  // BRAVE_COMPONENTS_SERVICES_BAT_ADS_NOTIFICATION_AD_MOJOM_UTIL_H_
