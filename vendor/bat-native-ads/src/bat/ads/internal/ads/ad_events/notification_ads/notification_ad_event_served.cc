/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/notification_ads/notification_ad_event_served.h"

#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ads/ad_events/ad_events.h"
#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/notification_ad_info.h"

namespace ads {
namespace notification_ads {

AdEventServed::AdEventServed() = default;

AdEventServed::~AdEventServed() = default;

void AdEventServed::FireEvent(const NotificationAdInfo& ad) {
  BLOG(3, "Served notification ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  LogAdEvent(ad, ConfirmationType::kServed, [](const bool success) {
    if (!success) {
      BLOG(1, "Failed to log notification ad served event");
      return;
    }

    BLOG(1, "Successfully logged notification ad served event");
  });
}

}  // namespace notification_ads
}  // namespace ads
