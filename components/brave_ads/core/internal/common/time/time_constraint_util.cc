/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/time/time_constraint_util.h"

#include "base/ranges/algorithm.h"
#include "base/time/time.h"

namespace brave_ads {

bool DoesHistoryRespectRollingTimeConstraint(
    const std::vector<base::Time>& history,
    const base::TimeDelta time_constraint,
    const size_t cap) {
  const size_t count = base::ranges::count_if(
      history, [time_constraint](const base::Time created_at) {
        return base::Time::Now() - created_at < time_constraint;
      });

  return count < cap;
}

}  // namespace brave_ads
