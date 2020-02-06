/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "bat/ads/internal/ad_events/publisher_ad_event_clicked.h"
#include "bat/ads/internal/reports.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/publisher_ad_info.h"
#include "bat/ads/confirmation_type.h"
#include "base/logging.h"

namespace ads {

PublisherAdEventClicked::PublisherAdEventClicked(
    AdsImpl* ads)
    : ads_(ads) {
}

PublisherAdEventClicked::~PublisherAdEventClicked() = default;

void PublisherAdEventClicked::Trigger(
    const PublisherAdInfo& info) {
  DCHECK(ads_);
  if (!ads_) {
    return;
  }

  Reports reports(ads_);
  const std::string report = reports.GeneratePublisherAdEventReport(info,
      PublisherAdEventType::kClicked);
  ads_->get_ads_client()->EventLog(report);

  ads_->ConfirmPublisherAd(info, ConfirmationType::kClicked);
}

}  // namespace ads
