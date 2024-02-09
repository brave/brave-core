/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/time/time_constraint_util.h"

#include "base/time/time.h"

namespace brave_ads {

bool DoesHistoryRespectRollingTimeConstraint(
    const std::vector<base::Time>& history,
    const base::TimeDelta time_constraint,
    size_t cap) {
  if (cap == 0) {
    // If the cap is 0, the limit has been reached.
    return false;
  }

  const base::Time threshold = base::Time::Now() - time_constraint;

  for (auto iter = history.crbegin(); iter != history.crend(); ++iter) {
    if (*iter <= threshold) {
      // If the time point exceeds the threshold, the cap has not been reached.
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

}  // namespace brave_ads
