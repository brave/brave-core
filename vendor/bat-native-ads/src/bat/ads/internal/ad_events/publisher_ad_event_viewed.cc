/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "bat/ads/publisher_ad_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/publisher_ad_event_viewed.h"
#include "bat/ads/internal/ads_impl.h"
#include "bat/ads/internal/reports.h"
#include "bat/ads/internal/time.h"
#include "base/logging.h"

namespace ads {

PublisherAdEventViewed::PublisherAdEventViewed(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

PublisherAdEventViewed::~PublisherAdEventViewed() = default;

void PublisherAdEventViewed::Trigger(
    const PublisherAdInfo& info) {
  Reports reports(ads_);
  const std::string report = reports.GeneratePublisherAdEventReport(
      info, PublisherAdEventType::kViewed);
  ads_->get_ads_client()->EventLog(report);

  ads_->set_last_shown_publisher_ad(info);

  const uint64_t now_in_seconds = Time::NowInSeconds();
  ads_->get_client()->AppendTimestampToCreativeSetHistory(
      info.creative_set_id, now_in_seconds);
  ads_->get_client()->AppendTimestampToCampaignHistory(
      info.creative_instance_id, now_in_seconds);

  ads_->get_client()->UpdateSeenPublisherAd(
      info.creative_instance_id, 1);

  ads_->ConfirmAd(info, ConfirmationType::kViewed);
}

}  // namespace ads
