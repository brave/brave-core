/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "bat/ads/internal/ad_events/ad_notification_event_dismissed.h"
#include "bat/ads/internal/reports.h"
#include "bat/ads/confirmation_type.h"
#include "base/logging.h"

namespace ads {

AdNotificationEventDismissed::AdNotificationEventDismissed(
    AdsImpl* ads)
    : ads_(ads) {
}

AdNotificationEventDismissed::~AdNotificationEventDismissed() = default;

void AdNotificationEventDismissed::Trigger(
    const AdNotificationInfo& info) {
  DCHECK(ads_);
  if (!ads_) {
    return;
  }

  Reports reports(ads_);
  const std::string report = reports.GenerateAdNotificationEventReport(info,
      AdNotificationEventType::kDismissed);
  ads_->get_ads_client()->EventLog(report);

  ads_->ConfirmAdNotification(info, ConfirmationType::kDismissed);

  ads_->AppendAdNotificationToAdsHistory(info, ConfirmationType::kDismissed);
}

}  // namespace ads
