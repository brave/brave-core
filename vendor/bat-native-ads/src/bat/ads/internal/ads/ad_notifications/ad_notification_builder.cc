/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_notifications/ad_notification_builder.h"

#include "base/guid.h"
#include "bat/ads/ad_notification_info.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"

namespace ads {

AdNotificationInfo BuildAdNotification(
    const CreativeAdNotificationInfo& creative_ad) {
  const std::string uuid = base::GenerateGUID();
  return BuildAdNotification(creative_ad, uuid);
}

AdNotificationInfo BuildAdNotification(
    const CreativeAdNotificationInfo& creative_ad,
    const std::string& uuid) {
  AdNotificationInfo ad;

  ad.type = AdType::kAdNotification;
  ad.uuid = uuid;
  ad.creative_instance_id = creative_ad.creative_instance_id;
  ad.creative_set_id = creative_ad.creative_set_id;
  ad.campaign_id = creative_ad.campaign_id;
  ad.advertiser_id = creative_ad.advertiser_id;
  ad.segment = creative_ad.segment;
  ad.title = creative_ad.title;
  ad.body = creative_ad.body;
  ad.target_url = creative_ad.target_url;

  return ad;
}

}  // namespace ads
