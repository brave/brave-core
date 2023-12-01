/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/allocation/seen_advertisers_util.h"

#include <optional>

#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/creatives/creative_ad_info.h"

namespace brave_ads {

std::optional<base::Time> GetLastSeenAdvertiserAt(
    const AdEventList& ad_events,
    const CreativeAdInfo& creative_ad) {
  const auto iter = base::ranges::find_if(
      ad_events, [&creative_ad](const AdEventInfo& ad_event) -> bool {
        return ad_event.confirmation_type == ConfirmationType::kViewed &&
               ad_event.advertiser_id == creative_ad.advertiser_id;
      });

  if (iter == ad_events.cend()) {
    return std::nullopt;
  }

  return iter->created_at;
}

}  // namespace brave_ads
