/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_notifications/ad_notification_event_timed_out.h"

#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/creatives/ad_notifications/ad_notifications.h"

namespace ads {
namespace ad_notifications {

AdEventTimedOut::AdEventTimedOut() = default;

AdEventTimedOut::~AdEventTimedOut() = default;

void AdEventTimedOut::FireEvent(const AdNotificationInfo& ad) {
  BLOG(3, "Timed out ad notification with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  AdNotifications::Get()->Remove(ad.placement_id);
}

}  // namespace ad_notifications
}  // namespace ads
