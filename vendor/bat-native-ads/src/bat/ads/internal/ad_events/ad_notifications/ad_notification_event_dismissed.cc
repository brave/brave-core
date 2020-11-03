/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_notifications/ad_notification_event_dismissed.h"

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/ads_history/ads_history.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_notifications {

namespace {
const ConfirmationType kConfirmationType = ConfirmationType::kDismissed;
}  // namespace

AdEventDismissed::AdEventDismissed(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdEventDismissed::~AdEventDismissed() = default;

void AdEventDismissed::Trigger(
    const AdNotificationInfo& ad) {
  BLOG(3, "Dismissed ad notification with uuid " << ad.uuid
      << " and creative instance id " << ad.creative_instance_id);

  ads_->get_ad_notifications()->Remove(ad.uuid, /* should dismiss */ false);

  AdEvents ad_events(ads_);
  ad_events.Log(ad, kConfirmationType, [](
      const Result result) {
    if (result != Result::SUCCESS) {
      BLOG(1, "Failed to log ad notification dismissed event");
      return;
    }

    BLOG(6, "Successfully logged ad notification dismissed event");
  });

  ads_->get_ads_history()->AddAdNotification(ad, kConfirmationType);

  ads_->get_confirmations()->ConfirmAd(ad.creative_instance_id,
      kConfirmationType);
}

}  // namespace ad_notifications
}  // namespace ads
