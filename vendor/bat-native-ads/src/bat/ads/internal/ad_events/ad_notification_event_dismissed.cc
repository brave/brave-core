/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_notification_event_dismissed.h"

#include <string>

#include "bat/ads/ad_notification_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_notifications/ad_notifications.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/confirmations/confirmations.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/reports/reports.h"

namespace ads {

AdNotificationEventDismissed::AdNotificationEventDismissed(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdNotificationEventDismissed::~AdNotificationEventDismissed() = default;

void AdNotificationEventDismissed::Trigger(
    const AdNotificationInfo& info) {
  ads_->get_ad_notifications()->Remove(info.uuid, false);

  Reports reports(ads_);
  const std::string report = reports.GenerateAdNotificationEventReport(info,
      AdNotificationEventType::kDismissed);
  BLOG(3, "Event log: " << report);

  ads_->get_confirmations()->ConfirmAd(info, ConfirmationType::kDismissed);

  ads_->AppendAdNotificationToHistory(info, ConfirmationType::kDismissed);
}

}  // namespace ads
