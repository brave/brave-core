/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/frequency_capping_util.h"

#include "base/time/time.h"

namespace ads {

std::deque<uint64_t> GetTimestampHistoryForAdEvents(
    const AdEventList& ad_events) {
  std::deque<uint64_t> history;

  for (const auto& ad_event : ad_events) {
    history.push_back(ad_event.timestamp);
  }

  return history;
}

bool DoesHistoryRespectCapForRollingTimeConstraint(
    const std::deque<uint64_t>& history,
    const uint64_t time_constraint_in_seconds,
    const uint64_t cap) {
  uint64_t count = 0;

  const uint64_t now_in_seconds =
      static_cast<uint64_t>(base::Time::Now().ToDoubleT());

  for (const auto& timestamp_in_seconds : history) {
    if (now_in_seconds - timestamp_in_seconds < time_constraint_in_seconds) {
      count++;
    }
  }

  if (count >= cap) {
    return false;
  }

  return true;
}

}  // namespace ads
