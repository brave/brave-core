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
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

namespace brave_ads {

namespace {

std::string BuildTypeId(const mojom::AdType mojom_ad_type,
                        const mojom::ConfirmationType mojom_confirmation_type) {
  return base::StrCat(
      {ToString(mojom_ad_type), ToString(mojom_confirmation_type)});
}

void PurgeCacheOlderThan(std::vector<base::Time>& cache,
                         const base::TimeDelta time_delta) {
  const base::Time past = base::Time::Now() - time_delta;

  std::erase_if(cache, [past](const base::Time time) { return time < past; });
}

}  // namespace

AdEventCache::AdEventCache() = default;

AdEventCache::~AdEventCache() = default;

void AdEventCache::AddEntryForInstanceId(
    const std::string& id,
    const mojom::AdType mojom_ad_type,
    const mojom::ConfirmationType mojom_confirmation_type,
    const base::Time time) {
  CHECK(!id.empty());
  CHECK_NE(mojom::AdType::kUndefined, mojom_ad_type);
  CHECK_NE(mojom::ConfirmationType::kUndefined, mojom_confirmation_type);

  const std::string type_id =
      BuildTypeId(mojom_ad_type, mojom_confirmation_type);

  ad_event_cache_[id][type_id].push_back(time);

  // Purge entries older than 1 day since this cache is utilized solely for
  // permission rules, which requires ad events from only the past day.
  PurgeCacheOlderThan(ad_event_cache_[id][type_id], base::Days(1));
}

std::vector<base::Time> AdEventCache::Get(
    const mojom::AdType mojom_ad_type,
    const mojom::ConfirmationType mojom_confirmation_type) const {
  CHECK_NE(mojom::AdType::kUndefined, mojom_ad_type);
  CHECK_NE(mojom::ConfirmationType::kUndefined, mojom_confirmation_type);

  const std::string type_id =
      BuildTypeId(mojom_ad_type, mojom_confirmation_type);

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
