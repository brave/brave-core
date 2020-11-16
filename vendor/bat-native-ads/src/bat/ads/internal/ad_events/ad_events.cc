/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_events.h"

#include <stdint.h>

#include "bat/ads/ad_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/time_util.h"

namespace ads {

AdEvents::AdEvents(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

AdEvents::~AdEvents() = default;

void AdEvents::Log(
    const AdInfo& ad,
    const ConfirmationType& confirmation_type,
    AdEventsCallback callback) {
  AdEventInfo ad_event;
  ad_event.type = ad.type;
  ad_event.uuid = ad.uuid;
  ad_event.creative_instance_id = ad.creative_instance_id;
  ad_event.creative_set_id = ad.creative_set_id;
  ad_event.campaign_id = ad.campaign_id;
  ad_event.timestamp = static_cast<int64_t>(base::Time::Now().ToDoubleT());
  ad_event.confirmation_type = confirmation_type;

  Log(ad_event, callback);
}

void AdEvents::Log(
    const AdEventInfo& ad_event,
    AdEventsCallback callback) {
  database::table::AdEvents database_table(ads_);
  database_table.LogEvent(ad_event, [callback](
      const Result result) {
    callback(result);
  });
}

void AdEvents::PurgeExpired(
    AdEventsCallback callback) {
  database::table::AdEvents database_table(ads_);
  database_table.PurgeExpired([callback](
      const Result result) {
    callback(result);
  });
}

}  // namespace ads
