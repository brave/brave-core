/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_notifications/ad_notification_event_clicked.h"

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_events.h"
#include "bat/ads/internal/ad_transfer/ad_transfer.h"
#include "bat/ads/internal/ads/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/ads_history/ads_history.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/logging.h"

namespace ads {
namespace ad_notifications {

namespace {
const ConfirmationType kConfirmationType = ConfirmationType::kClicked;
}  // namespace

AdEventClicked::AdEventClicked(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdEventClicked::~AdEventClicked() = default;

void AdEventClicked::Trigger(
    const AdNotificationInfo& ad) {
  BLOG(3, "Clicked ad notification with uuid " << ad.uuid
      << " and creative instance id " << ad.creative_instance_id);

  ads_->get_ad_notifications()->Remove(ad.uuid, /* should dismiss */ true);

  ads_->get_ad_transfer()->set_last_clicked_ad(ad);

  AdEvents ad_events(ads_);
  ad_events.Log(ad, kConfirmationType, [](
      const Result result) {
    if (result != Result::SUCCESS) {
      BLOG(1, "Failed to log ad notification clicked event");
      return;
    }

    BLOG(1, "Successfully logged ad notification clicked event");
  });

  ads_->get_ads_history()->AddAdNotification(ad, kConfirmationType);

  ads_->get_confirmations()->ConfirmAd(ad.creative_instance_id,
      kConfirmationType);
}

}  // namespace ad_notifications
}  // namespace ads
