/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_events.h"

#include <cstdint>

#include "base/time/time.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"

namespace ads {

void LogAdEvent(const AdInfo& ad,
                const ConfirmationType& confirmation_type,
                AdEventCallback callback) {
  AdEventInfo ad_event;
  ad_event.uuid = ad.uuid;
  ad_event.type = ad.type;
  ad_event.confirmation_type = confirmation_type;
  ad_event.campaign_id = ad.campaign_id;
  ad_event.creative_set_id = ad.creative_set_id;
  ad_event.creative_instance_id = ad.creative_instance_id;
  ad_event.advertiser_id = ad.advertiser_id;
  ad_event.timestamp = static_cast<int64_t>(base::Time::Now().ToDoubleT());

  LogAdEvent(ad_event, callback);
}

void LogAdEvent(const AdEventInfo& ad_event, AdEventCallback callback) {
  database::table::AdEvents database_table;
  database_table.LogEvent(
      ad_event, [callback](const Result result) { callback(result); });
}

void PurgeExpiredAdEvents(AdEventCallback callback) {
  database::table::AdEvents database_table;
  database_table.PurgeExpired(
      [callback](const Result result) { callback(result); });
}

}  // namespace ads
