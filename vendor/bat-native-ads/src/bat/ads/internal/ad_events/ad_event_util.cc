/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_events/ad_event_util.h"

#include "base/time/time.h"
#include "bat/ads/ad_info.h"
#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/bundle/creative_ad_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

bool HasFiredAdViewedEvent(const AdInfo& ad, const AdEventList& ad_events) {
  const auto iter = std::find_if(
      ad_events.begin(), ad_events.end(), [&ad](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type == ConfirmationType::kViewed &&
               ad_event.uuid == ad.uuid;
      });

  if (iter == ad_events.end()) {
    return false;
  }

  return true;
}

absl::optional<base::Time> GetLastSeenAdTime(
    const AdEventList& ad_events,
    const CreativeAdInfo& creative_ad) {
  const auto iter = std::find_if(
      ad_events.begin(), ad_events.end(),
      [&creative_ad](const AdEventInfo& ad_event) -> bool {
        return (ad_event.creative_instance_id ==
                    creative_ad.creative_instance_id &&
                ad_event.confirmation_type == ConfirmationType::kViewed);
      });

  if (iter == ad_events.end()) {
    return absl::nullopt;
  }

  return iter->created_at;
}

absl::optional<base::Time> GetLastSeenAdvertiserTime(
    const AdEventList& ad_events,
    const CreativeAdInfo& creative_ad) {
  const auto iter = std::find_if(
      ad_events.begin(), ad_events.end(),
      [&creative_ad](const AdEventInfo& ad_event) -> bool {
        return (ad_event.advertiser_id == creative_ad.advertiser_id &&
                ad_event.confirmation_type == ConfirmationType::kViewed);
      });

  if (iter == ad_events.end()) {
    return absl::nullopt;
  }

  return iter->created_at;
}

}  // namespace ads
