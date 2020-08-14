/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_notification_event_dismissed.h"

#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/logging.h"

namespace ads {

namespace {
const ConfirmationType kConfirmationType = ConfirmationType::kDismissed;
}  // namespace

AdNotificationEventDismissed::AdNotificationEventDismissed(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdNotificationEventDismissed::~AdNotificationEventDismissed() = default;

void AdNotificationEventDismissed::Trigger(
    const AdNotificationInfo& ad_notification) {
  BLOG(3, "Dismissed ad notification with uuid " << ad_notification.uuid
      << " and " << ad_notification.creative_instance_id
          << " creative instance id");

  ads_->get_ad_notifications()->Remove(ad_notification.uuid,
      /* should dismiss */ false);

  ads_->AppendAdNotificationToHistory(ad_notification, kConfirmationType);

  ads_->get_confirmations()->ConfirmAd(ad_notification, kConfirmationType);
}

}  // namespace ads
