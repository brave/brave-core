/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/notification_ads/notification_ad_event_factory.h"

#include "base/check.h"
#include "bat/ads/internal/ads/ad_events/notification_ads/notification_ad_event_clicked.h"
#include "bat/ads/internal/ads/ad_events/notification_ads/notification_ad_event_dismissed.h"
#include "bat/ads/internal/ads/ad_events/notification_ads/notification_ad_event_served.h"
#include "bat/ads/internal/ads/ad_events/notification_ads/notification_ad_event_timed_out.h"
#include "bat/ads/internal/ads/ad_events/notification_ads/notification_ad_event_viewed.h"
#include "bat/ads/notification_ad_info.h"

namespace ads::notification_ads {

std::unique_ptr<AdEventInterface<NotificationAdInfo>> AdEventFactory::Build(
    const mojom::NotificationAdEventType event_type) {
  DCHECK(mojom::IsKnownEnumValue(event_type));

  switch (event_type) {
    case mojom::NotificationAdEventType::kServed: {
      return std::make_unique<AdEventServed>();
    }

    case mojom::NotificationAdEventType::kViewed: {
      return std::make_unique<AdEventViewed>();
    }

    case mojom::NotificationAdEventType::kClicked: {
      return std::make_unique<AdEventClicked>();
    }

    case mojom::NotificationAdEventType::kDismissed: {
      return std::make_unique<AdEventDismissed>();
    }

    case mojom::NotificationAdEventType::kTimedOut: {
      return std::make_unique<AdEventTimedOut>();
    }
  }
}

}  // namespace ads::notification_ads
