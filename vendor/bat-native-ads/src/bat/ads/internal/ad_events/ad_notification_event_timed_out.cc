/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "bat/ads/internal/ad_events/ad_notification_event_timed_out.h"
#include "bat/ads/internal/reports.h"
#include "base/logging.h"

namespace ads {

AdNotificationEventTimedOut::AdNotificationEventTimedOut(
    AdsImpl* ads)
    : ads_(ads) {
}

AdNotificationEventTimedOut::~AdNotificationEventTimedOut() = default;

void AdNotificationEventTimedOut::Trigger(
    const AdNotificationInfo& info) {
  DCHECK(ads_);
  if (!ads_) {
    return;
  }

  Reports reports(ads_);
  const std::string report = reports.GenerateAdNotificationEventReport(info,
      AdNotificationEventType::kTimedOut);
  ads_->get_ads_client()->EventLog(report);
}

}  // namespace ads
