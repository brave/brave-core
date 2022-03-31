/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_delivery/ad_notifications/ad_notification_delivery.h"

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/ads_client_helper.h"

namespace ads {
namespace ad_notifications {

namespace {

void DeliverAd(const AdNotificationInfo& ad) {
  AdNotifications::Get()->PushBack(ad);

  AdsClientHelper::Get()->ShowNotification(ad);
}

}  // namespace

AdDelivery::AdDelivery() = default;

AdDelivery::~AdDelivery() = default;

bool AdDelivery::MaybeDeliverAd(const AdNotificationInfo& ad) {
  if (!ad.IsValid()) {
    return false;
  }

  DeliverAd(ad);

  return true;
}

}  // namespace ad_notifications
}  // namespace ads
