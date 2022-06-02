/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/notification_ads/notification_ad_event_clicked.h"

#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/deprecated/creatives/notification_ads/notification_ads.h"
#include "bat/ads/internal/history/history.h"

namespace ads {
namespace notification_ads {

AdEventClicked::AdEventClicked() = default;

AdEventClicked::~AdEventClicked() = default;

void AdEventClicked::FireEvent(const NotificationAdInfo& ad) {
  BLOG(3, "Clicked notification ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  NotificationAds::Get()->Remove(ad.placement_id);

  AdsClientHelper::Get()->CloseNotification(ad.placement_id);

  LogAdEvent(ad, ConfirmationType::kClicked, [](const bool success) {
    if (!success) {
      BLOG(1, "Failed to log notification ad clicked event");
      return;
    }

    BLOG(1, "Successfully logged notification ad clicked event");
  });

  history::AddNotificationAd(ad, ConfirmationType::kClicked);
}

}  // namespace notification_ads
}  // namespace ads
