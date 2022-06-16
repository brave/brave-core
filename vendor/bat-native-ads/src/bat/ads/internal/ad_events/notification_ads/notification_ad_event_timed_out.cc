/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/notification_ads/notification_ad_event_timed_out.h"

#include "bat/ads/internal/base/logging_util.h"
#include "bat/ads/internal/creatives/notification_ads/notification_ad_manager.h"

namespace ads {
namespace notification_ads {

AdEventTimedOut::AdEventTimedOut() = default;

AdEventTimedOut::~AdEventTimedOut() = default;

void AdEventTimedOut::FireEvent(const NotificationAdInfo& ad) {
  BLOG(3, "Timed out notification ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  NotificationAdManager::Get()->Remove(ad.placement_id);
}

}  // namespace notification_ads
}  // namespace ads
