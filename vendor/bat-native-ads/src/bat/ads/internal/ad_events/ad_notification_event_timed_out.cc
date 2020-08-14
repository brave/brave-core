/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_notification_event_timed_out.h"

#include "bat/ads/internal/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/logging.h"

namespace ads {

AdNotificationEventTimedOut::AdNotificationEventTimedOut(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdNotificationEventTimedOut::~AdNotificationEventTimedOut() = default;

void AdNotificationEventTimedOut::Trigger(
    const AdNotificationInfo& ad_notification) {
  BLOG(3, "Timed out ad notification with uuid " << ad_notification.uuid
      << " and " << ad_notification.creative_instance_id
          << " creative instance id");

  ads_->get_ad_notifications()->Remove(ad_notification.uuid,
      /* should dismiss */ false);
}

}  // namespace ads
