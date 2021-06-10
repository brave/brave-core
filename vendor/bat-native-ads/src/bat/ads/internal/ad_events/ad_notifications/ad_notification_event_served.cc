/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_notifications/ad_notification_event_served.h"

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/ads_history/ads_history.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_notifications {

AdEventServed::AdEventServed() = default;

AdEventServed::~AdEventServed() = default;

void AdEventServed::FireEvent(const AdNotificationInfo& ad) {
  BLOG(3, "Served ad notification with uuid " << ad.uuid
                                              << " and creative instance id "
                                              << ad.creative_instance_id);

  LogAdEvent(ad, ConfirmationType::kServed, [](const Result result) {
    if (result != Result::SUCCESS) {
      BLOG(1, "Failed to log ad notification served event");
      return;
    }

    BLOG(1, "Successfully logged ad notification served event");
  });
}

}  // namespace ad_notifications
}  // namespace ads
