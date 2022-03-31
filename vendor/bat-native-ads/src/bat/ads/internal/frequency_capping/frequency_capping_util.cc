/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"

#include "base/time/time.h"

namespace ads {

bool DoesHistoryRespectCapForRollingTimeConstraint(
    const std::deque<base::Time>& history,
    const base::TimeDelta time_constraint,
    const int cap) {
  int count = 0;

  const base::Time now = base::Time::Now();

  for (const auto& time : history) {
    if (now - time < time_constraint) {
      count++;
    }
  }

  if (count >= cap) {
    return false;
  }

  return true;
}

}  // namespace ads
