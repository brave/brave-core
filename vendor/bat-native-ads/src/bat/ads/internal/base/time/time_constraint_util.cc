/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/time/time_constraint_util.h"

#include <algorithm>

#include "base/time/time.h"

namespace ads {

bool DoesHistoryRespectRollingTimeConstraint(
    const std::vector<base::Time>& history,
    const base::TimeDelta time_constraint,
    const int cap) {
  const int count =
      std::count_if(history.cbegin(), history.cend(),
                    [time_constraint](const base::Time created_at) {
                      return base::Time::Now() - created_at < time_constraint;
                    });

  return count < cap;
}

}  // namespace ads
