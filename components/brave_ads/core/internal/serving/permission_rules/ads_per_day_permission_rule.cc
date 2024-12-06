/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/serving/permission_rules/ads_per_day_permission_rule.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_constraint_util.h"

namespace brave_ads {

bool HasAdsPerDayPermission(const std::vector<base::Time>& history,
                            size_t cap) {
  if (!DoesHistoryRespectRollingTimeConstraint(
          history, /*time_constraint=*/base::Days(1), cap)) {
    BLOG(2, "You have exceeded the allowed ads per day");
    return false;
  }

  return true;
}

}  // namespace brave_ads
