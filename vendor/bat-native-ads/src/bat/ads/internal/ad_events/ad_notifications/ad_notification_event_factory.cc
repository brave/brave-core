/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_notifications/ad_notification_event_factory.h"

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/internal/ad_events/ad_notifications/ad_notification_event_clicked.h"
#include "bat/ads/internal/ad_events/ad_notifications/ad_notification_event_dismissed.h"
#include "bat/ads/internal/ad_events/ad_notifications/ad_notification_event_served.h"
#include "bat/ads/internal/ad_events/ad_notifications/ad_notification_event_timed_out.h"
#include "bat/ads/internal/ad_events/ad_notifications/ad_notification_event_viewed.h"

namespace ads {
namespace ad_notifications {

std::unique_ptr<AdEventInterface<AdNotificationInfo>> AdEventFactory::Build(
    const mojom::AdNotificationEventType event_type) {
  switch (event_type) {
    case mojom::AdNotificationEventType::kServed: {
      return std::make_unique<AdEventServed>();
    }

    case mojom::AdNotificationEventType::kViewed: {
      return std::make_unique<AdEventViewed>();
    }

    case mojom::AdNotificationEventType::kClicked: {
      return std::make_unique<AdEventClicked>();
    }

    case mojom::AdNotificationEventType::kDismissed: {
      return std::make_unique<AdEventDismissed>();
    }

    case mojom::AdNotificationEventType::kTimedOut: {
      return std::make_unique<AdEventTimedOut>();
    }
  }
}

}  // namespace ad_notifications
}  // namespace ads
