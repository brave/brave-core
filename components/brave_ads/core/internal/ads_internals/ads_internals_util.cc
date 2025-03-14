/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_internals/ads_internals_util.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

namespace brave_ads {

namespace {

base::Value::List BuildCreativeSetConversions(
    const CreativeSetConversionList& creative_set_conversions) {
  base::Value::List list;
  list.reserve(creative_set_conversions.size());

  for (const auto& creative_set_conversion : creative_set_conversions) {
    if (!creative_set_conversion.IsValid()) {
      continue;
    }

    list.Append(base::Value::Dict()
                    .Set("URL Pattern", creative_set_conversion.url_pattern)
                    .Set("Expires At", creative_set_conversion.expire_at
                                           ->InSecondsFSinceUnixEpoch()));
  }

  return list;
}

base::Value::List BuildAdEvents(const AdEventList& ad_events) {
  base::Value::List list;
  list.reserve(ad_events.size());

  for (const auto& ad_event : ad_events) {
    if (!ad_event.IsValid()) {
      continue;
    }

    list.Append(base::Value::Dict()
                    .Set("Target URL", ad_event.target_url.spec())
                    .Set("Ad Type", ToString(ad_event.type))
                    .Set("Event Type", ToString(ad_event.confirmation_type))
                    .Set("Created At",
                         ad_event.created_at->InSecondsFSinceUnixEpoch()));
  }

  return list;
}

void GetUnexpiredAdEventsCallback(
    GetInternalsCallback callback,
    const CreativeSetConversionList& creative_set_conversions,
    bool success,
    const AdEventList& ad_events) {
  if (!success) {
    BLOG(0, "Failed to get ad events");
    return std::move(callback).Run({});
  }

  std::move(callback).Run(
      base::Value::Dict()
          .Set("creativeSetConversions",
               BuildCreativeSetConversions(creative_set_conversions))
          .Set("adEvents", BuildAdEvents(ad_events)));
}

void GetCreativeSetConversionsCallback(
    GetInternalsCallback callback,
    bool success,
    const CreativeSetConversionList& creative_set_conversions) {
  if (!success) {
    BLOG(0, "Failed to get creative set conversions");
    return std::move(callback).Run({});
  }

  database::table::AdEvents database_table;
  database_table.GetUnexpired(base::BindOnce(&GetUnexpiredAdEventsCallback,
                                             std::move(callback),
                                             creative_set_conversions));
}

}  // namespace

void BuildAdsInternals(GetInternalsCallback callback) {
  database::table::CreativeSetConversions database_table;
  database_table.GetActive(
      base::BindOnce(&GetCreativeSetConversionsCallback, std::move(callback)));
}

}  // namespace brave_ads
