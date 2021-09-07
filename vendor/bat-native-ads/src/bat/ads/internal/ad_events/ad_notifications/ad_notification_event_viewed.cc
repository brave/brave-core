/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_notifications/ad_notification_event_viewed.h"

#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ads_history/ads_history.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_notifications {

AdEventViewed::AdEventViewed() = default;

AdEventViewed::~AdEventViewed() = default;

void AdEventViewed::FireEvent(const AdNotificationInfo& ad) {
  BLOG(3, "Viewed ad notification with uuid " << ad.uuid
                                              << " and creative instance id "
                                              << ad.creative_instance_id);

  LogAdEvent(ad, ConfirmationType::kViewed, [](const bool success) {
    if (!success) {
      BLOG(1, "Failed to log ad notification viewed event");
      return;
    }

    BLOG(6, "Successfully logged ad notification viewed event");
  });

  history::AddAdNotification(ad, ConfirmationType::kViewed);
}

}  // namespace ad_notifications
}  // namespace ads
