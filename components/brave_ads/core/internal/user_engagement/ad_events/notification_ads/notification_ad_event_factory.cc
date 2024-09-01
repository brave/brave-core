/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/notification_ads/notification_ad_event_factory.h"

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/notification_ads/notification_ad_event_clicked.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/notification_ads/notification_ad_event_dismissed.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/notification_ads/notification_ad_event_served.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/notification_ads/notification_ad_event_timed_out.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/notification_ads/notification_ad_event_viewed.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"

namespace brave_ads {

std::unique_ptr<AdEventInterface<NotificationAdInfo>>
NotificationAdEventFactory::Build(
    const mojom::NotificationAdEventType mojom_ad_event_type) {
  switch (mojom_ad_event_type) {
    case mojom::NotificationAdEventType::kServedImpression: {
      return std::make_unique<NotificationAdEventServed>();
    }

    case mojom::NotificationAdEventType::kViewedImpression: {
      return std::make_unique<NotificationAdEventViewed>();
    }

    case mojom::NotificationAdEventType::kClicked: {
      return std::make_unique<NotificationAdEventClicked>();
    }

    case mojom::NotificationAdEventType::kDismissed: {
      return std::make_unique<NotificationAdEventDismissed>();
    }

    case mojom::NotificationAdEventType::kTimedOut: {
      return std::make_unique<NotificationAdEventTimedOut>();
    }
  }
}

}  // namespace brave_ads
