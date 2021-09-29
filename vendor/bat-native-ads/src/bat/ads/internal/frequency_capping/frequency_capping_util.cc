/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"

#include "base/time/time.h"

namespace ads {

std::deque<base::Time> GetHistoryForAdEvents(const AdEventList& ad_events) {
  std::deque<base::Time> history;

  for (const auto& ad_event : ad_events) {
    history.push_back(ad_event.created_at);
  }

  return history;
}

bool DoesHistoryRespectCapForRollingTimeConstraint(
    const std::deque<base::Time>& history,
    const base::TimeDelta& time_constraint,
    const uint64_t cap) {
  uint64_t count = 0;

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
