/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/creatives/notification_ads/notification_ad_builder.h"

#include "base/guid.h"
#include "bat/ads/internal/creatives/notification_ads/creative_notification_ad_info.h"
#include "bat/ads/notification_ad_info.h"

namespace ads {

NotificationAdInfo BuildNotificationAd(
    const CreativeNotificationAdInfo& creative_ad) {
  const std::string placement_id =
      base::GUID::GenerateRandomV4().AsLowercaseString();
  return BuildNotificationAd(creative_ad, placement_id);
}

NotificationAdInfo BuildNotificationAd(
    const CreativeNotificationAdInfo& creative_ad,
    const std::string& placement_id) {
  NotificationAdInfo ad;

  ad.type = AdType::kNotificationAd;
  ad.placement_id = placement_id;
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
