/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/time/time_constraint_util.h"

#include "base/containers/adapters.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

bool DoesHistoryRespectRollingTimeConstraint(
    const std::vector<base::Time>& history,
    const base::TimeDelta time_constraint,
    size_t cap) {
  if (cap == 0) {
    // If the cap is set to 0, then there is no time constraint.
    return true;
  }

  const base::Time threshold = base::Time::Now() - time_constraint;

  for (const auto& time : base::Reversed(history)) {
    if (time <= threshold) {
      // If the time point is less than or equal to the threshold, the cap has
      // not been reached.
      break;
    }

    --cap;
    if (cap == 0) {
      // If the cap is hit, the limit has been reached.
      return false;
    }
  }

  return true;
}

bool DoesHistoryRespectRollingTimeConstraint(
    const mojom::AdType mojom_ad_type,
    const base::TimeDelta time_constraint,
    const size_t cap) {
  const std::vector<base::Time> history = GetAdsClient().GetCachedAdEvents(
      mojom_ad_type, mojom::ConfirmationType::kServedImpression);

  return DoesHistoryRespectRollingTimeConstraint(history, time_constraint, cap);
}

}  // namespace brave_ads
