/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "bat/ads/internal/ad_events/ad_notification_event_viewed.h"
#include "bat/ads/internal/reports.h"
#include "bat/ads/confirmation_type.h"
#include "base/logging.h"

namespace ads {

AdNotificationEventViewed::AdNotificationEventViewed(
    AdsImpl* ads)
    : ads_(ads) {
}

AdNotificationEventViewed::~AdNotificationEventViewed() = default;

void AdNotificationEventViewed::Trigger(
    const AdNotificationInfo& info) {
  DCHECK(ads_);
  if (!ads_) {
    return;
  }

  Reports reports(ads_);
  const std::string report = reports.GenerateAdNotificationEventReport(info,
      AdNotificationEventType::kViewed);
  ads_->get_ads_client()->EventLog(report);

  ads_->ConfirmAdNotification(info, ConfirmationType::kViewed);

  ads_->AppendAdNotificationToAdsHistory(info, ConfirmationType::kViewed);
}

}  // namespace ads
