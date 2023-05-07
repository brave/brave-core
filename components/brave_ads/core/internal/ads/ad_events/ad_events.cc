/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_events.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/ad_info.h"
#include "brave/components/brave_ads/core/ad_type.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/common/instance_id_constants.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"

namespace brave_ads {

void LogAdEvent(const AdInfo& ad,
                const ConfirmationType& confirmation_type,
                AdEventCallback callback) {
  AdEventInfo ad_event;
  ad_event.type = ad.type;
  ad_event.confirmation_type = confirmation_type;
  ad_event.placement_id = ad.placement_id;
  ad_event.campaign_id = ad.campaign_id;
  ad_event.creative_set_id = ad.creative_set_id;
  ad_event.creative_instance_id = ad.creative_instance_id;
  ad_event.advertiser_id = ad.advertiser_id;
  ad_event.segment = ad.segment;
  ad_event.created_at = base::Time::Now();

  LogAdEvent(ad_event, std::move(callback));
}

void LogAdEvent(const AdEventInfo& ad_event, AdEventCallback callback) {
  RecordAdEvent(ad_event);

  database::table::AdEvents database_table;
  database_table.LogEvent(ad_event,
                          base::BindOnce(
                              [](AdEventCallback callback, const bool success) {
                                std::move(callback).Run(success);
                              },
                              std::move(callback)));
}

void PurgeExpiredAdEvents(AdEventCallback callback) {
  const database::table::AdEvents database_table;
  database_table.PurgeExpired(base::BindOnce(
      [](AdEventCallback callback, const bool success) {
        std::move(callback).Run(success);
      },
      std::move(callback)));
}

void PurgeOrphanedAdEvents(const mojom::AdType ad_type,
                           AdEventCallback callback) {
  CHECK(mojom::IsKnownEnumValue(ad_type));

  const database::table::AdEvents database_table;
  database_table.PurgeOrphaned(
      ad_type, base::BindOnce(
                   [](AdEventCallback callback, const bool success) {
                     std::move(callback).Run(success);
                   },
                   std::move(callback)));
}

void RebuildAdEventHistoryFromDatabase() {
  const database::table::AdEvents database_table;
  database_table.GetAll(
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        if (!success) {
          BLOG(1, "Failed to get ad events");
          return;
        }

        const std::string& id = GetInstanceId();

        AdsClientHelper::GetInstance()->ResetAdEventHistoryForId(id);

        for (const auto& ad_event : ad_events) {
          RecordAdEvent(ad_event);
        }
      }));
}

void RecordAdEvent(const AdEventInfo& ad_event) {
  AdsClientHelper::GetInstance()->RecordAdEventForId(
      GetInstanceId(), ad_event.type.ToString(),
      ad_event.confirmation_type.ToString(), ad_event.created_at);
}

std::vector<base::Time> GetAdEventHistory(
    const AdType& ad_type,
    const ConfirmationType& confirmation_type) {
  return AdsClientHelper::GetInstance()->GetAdEventHistory(
      ad_type.ToString(), confirmation_type.ToString());
}

}  // namespace brave_ads
