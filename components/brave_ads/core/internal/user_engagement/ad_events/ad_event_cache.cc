/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/user_engagement/ad_events/ad_event_cache.h"

#include "base/check.h"
#include "base/containers/extend.h"
#include "base/containers/flat_map.h"
#include "base/strings/strcat.h"
#include "base/time/time.h"

namespace brave_ads {

namespace {

std::string BuildTypeId(const std::string& ad_type,
                        const std::string& confirmation_type) {
  return base::StrCat({ad_type, confirmation_type});
}

void PurgeCacheOlderThan(std::vector<base::Time>& cache,
                         const base::TimeDelta time_delta) {
  const base::Time past = base::Time::Now() - time_delta;

  std::erase_if(cache, [past](const base::Time time) { return time < past; });
}

}  // namespace

AdEventCache::AdEventCache() = default;

AdEventCache::~AdEventCache() = default;

void AdEventCache::AddEntryForInstanceId(const std::string& id,
                                         const std::string& ad_type,
                                         const std::string& confirmation_type,
                                         const base::Time time) {
  CHECK(!id.empty());
  CHECK(!ad_type.empty());
  CHECK(!confirmation_type.empty());

  const std::string type_id = BuildTypeId(ad_type, confirmation_type);

  ad_event_cache_[id][type_id].push_back(time);

  // Purge entries older than 1 day since this cache is utilized solely for
  // permission rules, which requires ad events from only the past day.
  PurgeCacheOlderThan(ad_event_cache_[id][type_id], base::Days(1));
}

std::vector<base::Time> AdEventCache::Get(
    const std::string& ad_type,
    const std::string& confirmation_type) const {
  CHECK(!ad_type.empty());
  CHECK(!confirmation_type.empty());

  const std::string type_id = BuildTypeId(ad_type, confirmation_type);

  std::vector<base::Time> timestamps;

  for (const auto& [_, ad_event_history] : ad_event_cache_) {
    for (const auto& [ad_event_type_id, ad_event_timestamps] :
         ad_event_history) {
      if (ad_event_type_id == type_id) {
        base::Extend(timestamps, ad_event_timestamps);
      }
    }
  }

  return timestamps;
}

void AdEventCache::ResetForInstanceId(const std::string& id) {
  ad_event_cache_[id].clear();
}

}  // namespace brave_ads
