/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_notifications/ad_notification_event_viewed.h"

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ads_history/ads_history.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_notifications {

namespace {
const ConfirmationType kConfirmationType = ConfirmationType::kViewed;
}  // namespace

AdEventViewed::AdEventViewed(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdEventViewed::~AdEventViewed() = default;

void AdEventViewed::Trigger(
    const AdNotificationInfo& ad) {
  BLOG(3, "Viewed ad notification with uuid " << ad.uuid
      << " and creative instance id " << ad.creative_instance_id);

  AdEvents ad_events(ads_);
  ad_events.Log(ad, kConfirmationType, [](
      const Result result) {
    if (result != Result::SUCCESS) {
      BLOG(1, "Failed to log ad notification viewed event");
      return;
    }

    BLOG(6, "Successfully logged ad notification viewed event");
  });

  ads_->get_ads_history()->AddAdNotification(ad, kConfirmationType);

  ads_->get_confirmations()->ConfirmAd(ad.creative_instance_id,
      kConfirmationType);
}

}  // namespace ad_notifications
}  // namespace ads
