/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_cache_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

namespace brave_ads {

void RecordAdEvent(const AdInfo& ad,
                   const ConfirmationType confirmation_type,
                   AdEventCallback callback) {
  RecordAdEvent(
      BuildAdEvent(ad, confirmation_type, /*created_at=*/base::Time::Now()),
      std::move(callback));
}

void RecordAdEvent(const AdEventInfo& ad_event, AdEventCallback callback) {
  CacheAdEvent(ad_event);

  database::table::AdEvents database_table;
  database_table.RecordEvent(
      ad_event, base::BindOnce(
                    [](AdEventCallback callback, const bool success) {
                      std::move(callback).Run(success);
                    },
                    std::move(callback)));
}

void PurgeOrphanedAdEvents(const mojom::AdType mojom_ad_type,
                           AdEventCallback callback) {
  const database::table::AdEvents database_table;
  database_table.PurgeOrphaned(
      mojom_ad_type, base::BindOnce(
                         [](AdEventCallback callback, const bool success) {
                           if (success) {
                             RebuildAdEventCache();
                           }

                           std::move(callback).Run(success);
                         },
                         std::move(callback)));
}

void PurgeOrphanedAdEvents(const std::vector<std::string>& placement_ids,
                           AdEventCallback callback) {
  const database::table::AdEvents database_table;
  database_table.PurgeOrphaned(
      placement_ids, base::BindOnce(
                         [](AdEventCallback callback, const bool success) {
                           if (success) {
                             RebuildAdEventCache();
                           }

                           std::move(callback).Run(success);
                         },
                         std::move(callback)));
}

}  // namespace brave_ads
