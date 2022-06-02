/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/serving/delivery/notification_ads/notification_ad_delivery.h"

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/deprecated/creatives/notification_ads/notification_ads.h"
#include "bat/ads/notification_ad_info.h"

namespace ads {
namespace notification_ads {

namespace {

void DeliverAd(const NotificationAdInfo& ad) {
  NotificationAds::Get()->PushBack(ad);

  AdsClientHelper::Get()->ShowNotification(ad);
}

}  // namespace

Delivery::Delivery() = default;

Delivery::~Delivery() = default;

bool Delivery::MaybeDeliverAd(const NotificationAdInfo& ad) {
  if (!ad.IsValid()) {
    return false;
  }

  DeliverAd(ad);

  return true;
}

}  // namespace notification_ads
}  // namespace ads
