/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/eligible_ads/allocation/seen_ads_util.h"

#include "base/ranges/algorithm.h"
#include "base/time/time.h"

namespace brave_ads {

std::optional<base::Time> GetLastSeenAdAt(
    const AdEventList& ad_events,
    const std::string& creative_instance_id) {
  const auto iter = base::ranges::find_if(
      ad_events, [&creative_instance_id](const AdEventInfo& ad_event) {
        return ad_event.confirmation_type ==
                   mojom::ConfirmationType::kViewedImpression &&
               ad_event.creative_instance_id == creative_instance_id;
      });

  if (iter == ad_events.cend()) {
    return std::nullopt;
  }

  return iter->created_at;
}

}  // namespace brave_ads
