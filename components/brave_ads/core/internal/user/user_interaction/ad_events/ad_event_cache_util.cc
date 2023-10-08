/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_cache_util.h"

#include <string>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/instance_id.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/units/ad_type.h"

namespace brave_ads {

void RebuildAdEventCache() {
  const database::table::AdEvents database_table;
  database_table.GetAll(
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        if (!success) {
          return BLOG(1, "Failed to get ad events");
        }

        const std::string& id = GetInstanceId();

        ResetAdEventCacheForInstanceId(id);

        for (const auto& ad_event : ad_events) {
          if (ad_event.IsValid()) {
            CacheAdEvent(ad_event);
          }
        }
      }));
}

void CacheAdEvent(const AdEventInfo& ad_event) {
  CacheAdEventForInstanceId(GetInstanceId(), ad_event.type.ToString(),
                            ad_event.confirmation_type.ToString(),
                            ad_event.created_at);
}

std::vector<base::Time> GetCachedAdEvents(
    const AdType& ad_type,
    const ConfirmationType& confirmation_type) {
  return GetCachedAdEvents(ad_type.ToString(), confirmation_type.ToString());
}

void ResetAdEventCache() {
  ResetAdEventCacheForInstanceId(GetInstanceId());
}

}  // namespace brave_ads
