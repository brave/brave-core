/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_events.h"

#include <string>
#include <vector>

#include "base/time/time.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/ad_type.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/database/tables/ad_events_database_table.h"
#include "bat/ads/internal/instance_id_util.h"
#include "bat/ads/internal/logging.h"

namespace ads {

void LogAdEvent(const AdInfo& ad,
                const ConfirmationType& confirmation_type,
                AdEventCallback callback) {
  AdEventInfo ad_event;
  ad_event.uuid = ad.placement_id;
  ad_event.type = ad.type;
  ad_event.confirmation_type = confirmation_type;
  ad_event.campaign_id = ad.campaign_id;
  ad_event.creative_set_id = ad.creative_set_id;
  ad_event.creative_instance_id = ad.creative_instance_id;
  ad_event.advertiser_id = ad.advertiser_id;
  ad_event.created_at = base::Time::Now();

  LogAdEvent(ad_event, callback);
}

void LogAdEvent(const AdEventInfo& ad_event, AdEventCallback callback) {
  RecordAdEvent(ad_event);

  database::table::AdEvents database_table;
  database_table.LogEvent(
      ad_event, [callback](const bool success) { callback(success); });
}

void PurgeExpiredAdEvents(AdEventCallback callback) {
  database::table::AdEvents database_table;
  database_table.PurgeExpired([callback](const bool success) {
    if (success) {
      RebuildAdEventsFromDatabase();
    }

    callback(success);
  });
}

void PurgeOrphanedAdEvents(const mojom::AdType ad_type,
                           AdEventCallback callback) {
  database::table::AdEvents database_table;
  database_table.PurgeOrphaned(ad_type, [callback](const bool success) {
    if (success) {
      RebuildAdEventsFromDatabase();
    }

    callback(success);
  });
}

void RebuildAdEventsFromDatabase() {
  database::table::AdEvents database_table;
  database_table.GetAll([=](const bool success, const AdEventList& ad_events) {
    if (!success) {
      BLOG(1, "Failed to get ad events");
      return;
    }

    const std::string& id = GetInstanceId();

    AdsClientHelper::Get()->ResetAdEventsForId(id);

    for (const auto& ad_event : ad_events) {
      RecordAdEvent(ad_event);
    }
  });
}

void RecordAdEvent(const AdEventInfo& ad_event) {
  const std::string& id = GetInstanceId();

  const AdType& ad_type = ad_event.type;
  const ConfirmationType& confirmation_type = ad_event.confirmation_type;

  const double timestamp = ad_event.created_at.ToDoubleT();

  AdsClientHelper::Get()->RecordAdEventForId(
      id, ad_type.ToString(), confirmation_type.ToString(), timestamp);
}

std::deque<base::Time> GetAdEvents(const AdType& ad_type,
                                   const ConfirmationType& confirmation_type) {
  const std::vector<double>& history = AdsClientHelper::Get()->GetAdEvents(
      ad_type.ToString(), confirmation_type.ToString());

  std::deque<base::Time> deque;

  for (const auto& timestamp : history) {
    const base::Time time = base::Time::FromDoubleT(timestamp);
    deque.push_back(time);
  }

  return deque;
}

}  // namespace ads
