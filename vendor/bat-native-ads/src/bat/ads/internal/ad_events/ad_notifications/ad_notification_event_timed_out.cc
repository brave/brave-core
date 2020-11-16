/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_notifications/ad_notification_event_timed_out.h"

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_notifications {

AdEventTimedOut::AdEventTimedOut(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdEventTimedOut::~AdEventTimedOut() = default;

void AdEventTimedOut::Trigger(
    const AdNotificationInfo& ad) {
  BLOG(3, "Timed out ad notification with uuid " << ad.uuid
      << " and creative instance id " << ad.creative_instance_id);

  ads_->get_ad_notifications()->Remove(ad.uuid, /* should dismiss */ false);
}

}  // namespace ad_notifications
}  // namespace ads
