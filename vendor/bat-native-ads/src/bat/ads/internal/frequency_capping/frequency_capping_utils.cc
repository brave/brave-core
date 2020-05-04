/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/frequency_capping/frequency_capping_utils.h"

#include "base/time/time.h"

namespace ads {

bool DoesHistoryRespectCapForRollingTimeConstraint(
    const std::deque<uint64_t> history,
    const uint64_t time_constraint_in_seconds,
    const uint64_t cap) {
  uint64_t count = 0;

  const uint64_t now_in_seconds = base::Time::Now().ToDoubleT();

  for (const auto& timestamp_in_seconds : history) {
    if (now_in_seconds - timestamp_in_seconds < time_constraint_in_seconds) {
      count++;
    }
  }

  if (count < cap) {
    return true;
  }

  return false;
}

}  // namespace ads
