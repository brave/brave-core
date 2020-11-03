/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_notifications/ad_notification.h"

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/internal/ad_events/ad_notifications/ad_notification_event_factory.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/logging.h"

namespace ads {

AdNotification::AdNotification(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdNotification::~AdNotification() = default;

void AdNotification::Trigger(
    const std::string& uuid,
    const AdNotificationEventType event_type) {
  DCHECK(!uuid.empty());

  AdNotificationInfo ad_notification;
  if (!ads_->get_ad_notifications()->Get(uuid, &ad_notification)) {
    BLOG(1, "Failed to trigger ad notification event for uuid " << uuid);
    return;
  }

  const auto ad_event =
      ad_notifications::AdEventFactory::Build(ads_, event_type);
  ad_event->Trigger(ad_notification);
}

}  // namespace ads
