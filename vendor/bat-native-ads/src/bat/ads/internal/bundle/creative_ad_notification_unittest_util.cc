/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/creative_ad_notification_unittest_util.h"

#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/bundle/creative_ad_unittest_util.h"

namespace ads {

CreativeAdNotificationInfo GetCreativeAdNotification() {
  const CreativeAdInfo creative_ad = GetCreativeAd();
  CreativeAdNotificationInfo creative_ad_notification(creative_ad);

  creative_ad_notification.title = "Test Ad Title";
  creative_ad_notification.body = "Test Ad Body";

  return creative_ad_notification;
}

CreativeAdNotificationInfo GetCreativeAdNotification(const std::string& segment,
                                                     const double ptr,
                                                     const int priority) {
  CreativeAdNotificationInfo creative_ad_notification =
      GetCreativeAdNotification();
  creative_ad_notification.priority = priority;
  creative_ad_notification.ptr = ptr;
  creative_ad_notification.segment = segment;

  return creative_ad_notification;
}

CreativeAdNotificationInfo GetCreativeAdNotification(
    const std::string& creative_instance_id,
    const std::string& segment) {
  CreativeAdNotificationInfo creative_ad_notification =
      GetCreativeAdNotification();

  creative_ad_notification.creative_instance_id = creative_instance_id;
  creative_ad_notification.segment = segment;

  return creative_ad_notification;
}

CreativeAdNotificationInfo GetCreativeAdNotificationForAdvertiser(
    const std::string& advertiser_id) {
  CreativeAdNotificationInfo creative_ad_notification =
      GetCreativeAdNotification();

  creative_ad_notification.advertiser_id = advertiser_id;

  return creative_ad_notification;
}

CreativeAdNotificationInfo GetCreativeAdNotificationForSegment(
    const std::string& segment) {
  CreativeAdNotificationInfo creative_ad_notification =
      GetCreativeAdNotification();

  creative_ad_notification.segment = segment;

  return creative_ad_notification;
}

}  // namespace ads
