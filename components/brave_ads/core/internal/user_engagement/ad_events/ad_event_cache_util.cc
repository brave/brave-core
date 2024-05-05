/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_cache_util.h"

#include <string>

#include "base/check.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/instance_id.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"

namespace brave_ads {

void RebuildAdEventCache() {
  const database::table::AdEvents database_table;
  database_table.GetUnexpired(
      base::BindOnce([](const bool success, const AdEventList& ad_events) {
        if (!success) {
          return BLOG(1, "Failed to get ad events");
        }

        const std::string& id = GetInstanceId();

        ResetAdEventCacheForInstanceId(id);

        for (const auto& ad_event : ad_events) {
          if (!ad_event.IsValid()) {
            // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
            // potential defects using `DumpWithoutCrashing`.
            base::debug::DumpWithoutCrashing();
            continue;
          }

          CacheAdEvent(ad_event);
        }
      }));
}

void CacheAdEvent(const AdEventInfo& ad_event) {
  CHECK(ad_event.IsValid());

  CacheAdEventForInstanceId(GetInstanceId(), ad_event.type,
                            ad_event.confirmation_type, *ad_event.created_at);
}

void ResetAdEventCache() {
  ResetAdEventCacheForInstanceId(GetInstanceId());
}

}  // namespace brave_ads
