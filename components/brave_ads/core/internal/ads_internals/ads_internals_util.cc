/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_internals/ads_internals_util.h"

#include <optional>
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

base::ListValue BuildCreativeSetConversions(
    const CreativeSetConversionList& creative_set_conversions) {
  base::ListValue list;
  list.reserve(creative_set_conversions.size());

  for (const auto& creative_set_conversion : creative_set_conversions) {
    if (!creative_set_conversion.IsValid()) {
      // Skip invalid creative set conversions.
      continue;
    }

    list.Append(base::DictValue()
                    .Set("URL Pattern", creative_set_conversion.url_pattern)
                    .Set("Expires At", creative_set_conversion.expire_at
                                           ->InSecondsFSinceUnixEpoch()));
  }

  return list;
}

base::ListValue BuildAdEvents(const AdEventList& ad_events) {
  base::ListValue list;
  list.reserve(ad_events.size());

  for (const auto& ad_event : ad_events) {
    if (!ad_event.IsValid()) {
      // Skip invalid ad events.
      continue;
    }

    if (ad_event.confirmation_type ==
        mojom::ConfirmationType::kServedImpression) {
      // Skip served impressions.
      continue;
    }

    list.Append(base::DictValue()
                    .Set("Target URL", ad_event.target_url.spec())
                    .Set("Ad Type", ToString(ad_event.type))
                    .Set("Event Type", ToString(ad_event.confirmation_type))
                    .Set("Created At",
                         ad_event.created_at->InSecondsFSinceUnixEpoch()));
  }

  return list;
}

void Successful(GetInternalsCallback callback,
                const CreativeSetConversionList& creative_set_conversions,
                const AdEventList& ad_events) {
  std::move(callback).Run(
      base::DictValue()
          .Set("creativeSetConversions",
               BuildCreativeSetConversions(creative_set_conversions))
          .Set("adEvents", BuildAdEvents(ad_events)));
}

void Failed(GetInternalsCallback callback) {
  BLOG(0, "Failed to get ads internals");
  std::move(callback).Run(/*internals=*/std::nullopt);
}

void GetUnexpiredAdEventsCallback(
    GetInternalsCallback callback,
    const CreativeSetConversionList& creative_set_conversions,
    bool success,
    const AdEventList& ad_events) {
  if (!success) {
    return Failed(std::move(callback));
  }

  Successful(std::move(callback), creative_set_conversions, ad_events);
}

void GetCreativeSetConversionsCallback(
    GetInternalsCallback callback,
    bool success,
    const CreativeSetConversionList& creative_set_conversions) {
  if (!success) {
    return Failed(std::move(callback));
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
