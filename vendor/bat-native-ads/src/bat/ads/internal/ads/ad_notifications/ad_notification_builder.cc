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
    const CreativeAdNotificationInfo& creative_ad_notification) {
  const std::string uuid = base::GenerateGUID();
  return BuildAdNotification(creative_ad_notification, uuid);
}

AdNotificationInfo BuildAdNotification(
    const CreativeAdNotificationInfo& creative_ad_notification,
    const std::string& uuid) {
  AdNotificationInfo ad_notification;

  ad_notification.type = AdType::kAdNotification;
  ad_notification.uuid = uuid;
  ad_notification.creative_instance_id =
      creative_ad_notification.creative_instance_id;
  ad_notification.creative_set_id = creative_ad_notification.creative_set_id;
  ad_notification.campaign_id = creative_ad_notification.campaign_id;
  ad_notification.advertiser_id = creative_ad_notification.advertiser_id;
  ad_notification.segment = creative_ad_notification.segment;
  ad_notification.title = creative_ad_notification.title;
  ad_notification.body = creative_ad_notification.body;
  ad_notification.target_url = creative_ad_notification.target_url;

  return ad_notification;
}

}  // namespace ads
