/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_notification_event_factory.h"
#include "bat/ads/internal/ad_events/ad_notification_event_viewed.h"
#include "bat/ads/internal/ad_events/ad_notification_event_clicked.h"
#include "bat/ads/internal/ad_events/ad_notification_event_dismissed.h"
#include "bat/ads/internal/ad_events/ad_notification_event_timed_out.h"

namespace ads {

std::unique_ptr<AdEvent<AdNotificationInfo>> AdEventFactory::Build(
    AdsImpl* ads,
    const AdNotificationEventType event_type) {
  DCHECK(ads);

  switch (event_type) {
    case AdNotificationEventType::kViewed: {
      return std::make_unique<AdNotificationEventViewed>(ads);
    }

    case AdNotificationEventType::kClicked: {
      return std::make_unique<AdNotificationEventClicked>(ads);
    }

    case AdNotificationEventType::kDismissed: {
      return std::make_unique<AdNotificationEventDismissed>(ads);
    }

    case AdNotificationEventType::kTimedOut: {
      return std::make_unique<AdNotificationEventTimedOut>(ads);
    }
  }
}

}  // namespace ads
