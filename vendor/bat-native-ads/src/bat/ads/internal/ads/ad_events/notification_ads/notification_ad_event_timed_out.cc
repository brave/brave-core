/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/notification_ads/notification_ad_event_timed_out.h"

#include "bat/ads/internal/common/logging_util.h"
#include "bat/ads/notification_ad_info.h"

namespace ads::notification_ads {

void AdEventTimedOut::FireEvent(const NotificationAdInfo& ad) {
  BLOG(3, "Timed out notification ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);
}

}  // namespace ads::notification_ads
