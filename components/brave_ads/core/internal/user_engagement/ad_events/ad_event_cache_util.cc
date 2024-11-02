/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_cache_util.h"

#include <string>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/instance_id.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

void RebuildAdEventCache() {
  const database::table::AdEvents database_table;
  database_table.GetUnexpired(
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        if (!success) {
          return BLOG(0, "Failed to get ad events");
        }

        const std::string& id = GetInstanceId();

        GetAdsClient().ResetAdEventCacheForInstanceId(id);

        for (const auto& ad_event : ad_events) {
          if (!ad_event.IsValid()) {
            BLOG(0, "Invalid ad event");

            continue;
          }

          CacheAdEvent(ad_event);
        }
      }));
}

void CacheAdEvent(const AdEventInfo& ad_event) {
  CHECK(ad_event.IsValid());

  GetAdsClient().CacheAdEventForInstanceId(GetInstanceId(), ad_event.type,
                                           ad_event.confirmation_type,
                                           *ad_event.created_at);
}

void ResetAdEventCache() {
  GetAdsClient().ResetAdEventCacheForInstanceId(GetInstanceId());
}

}  // namespace brave_ads
