/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ads/ad_events/ad_event_util.h"

#include "base/ranges/algorithm.h"
#include "base/time/time.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/confirmation_type.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"

namespace ads {

bool HasFiredAdEvent(const AdInfo& ad,
                     const AdEventList& ad_events,
                     const ConfirmationType& confirmation_type) {
  const auto iter = base::ranges::find_if(
      ad_events, [&ad, &confirmation_type](const AdEventInfo& ad_event) {
        return ad_event.placement_id == ad.placement_id &&
               ad_event.confirmation_type == confirmation_type;
      });

  return iter != ad_events.cend();
}

absl::optional<base::Time> GetLastSeenAdTime(
    const AdEventList& ad_events,
    const CreativeAdInfo& creative_ad) {
  const auto iter = base::ranges::find_if(
      ad_events, [&creative_ad](const AdEventInfo& ad_event) -> bool {
        return ad_event.creative_instance_id ==
                   creative_ad.creative_instance_id &&
               ad_event.confirmation_type == ConfirmationType::kViewed;
      });

  if (iter == ad_events.cend()) {
    return absl::nullopt;
  }

  return iter->created_at;
}

absl::optional<base::Time> GetLastSeenAdvertiserTime(
    const AdEventList& ad_events,
    const CreativeAdInfo& creative_ad) {
  const auto iter = base::ranges::find_if(
      ad_events, [&creative_ad](const AdEventInfo& ad_event) -> bool {
        return ad_event.advertiser_id == creative_ad.advertiser_id &&
               ad_event.confirmation_type == ConfirmationType::kViewed;
      });

  if (iter == ad_events.cend()) {
    return absl::nullopt;
  }

  return iter->created_at;
}

}  // namespace ads
