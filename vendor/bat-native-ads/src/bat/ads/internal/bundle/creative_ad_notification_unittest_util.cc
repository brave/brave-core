/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/bundle/creative_ad_notification_unittest_util.h"

#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "bat/ads/internal/bundle/creative_ad_notification_info.h"
#include "bat/ads/internal/bundle/creative_ad_unittest_util.h"

namespace ads {

CreativeAdNotificationInfo BuildCreativeAdNotification() {
  const CreativeAdInfo creative_ad = BuildCreativeAd();
  CreativeAdNotificationInfo creative_ad_notification(creative_ad);

  creative_ad_notification.title = "Test Ad Title";
  creative_ad_notification.body = "Test Ad Body";

  return creative_ad_notification;
}

}  // namespace ads
