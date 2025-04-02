/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/timer/backoff_timer.h"

#include <limits>
#include <utility>

namespace brave_ads {

// The `kMaxBackoffCount` constant is calculated as the number of bits in an
// `int64_t` minus one. This ensures that the backoff count does not exceed
// the maximum number of left shifts possible for a 64-bit integer, preventing
// potential overflow issues during delay calculations.
constexpr int64_t kMaxBackoffCount = (std::numeric_limits<int64_t>::digits) - 1;

base::Time BackoffTimer::Start(const base::Location& location,
                               base::TimeDelta delay,
                               base::OnceClosure user_task) {
  timer_.Stop();

  return timer_.Start(location, CalculateDelay(delay), std::move(user_task));
}

base::Time BackoffTimer::StartWithPrivacy(const base::Location& location,
                                          base::TimeDelta delay,
                                          base::OnceClosure user_task) {
  timer_.Stop();

  return timer_.StartWithPrivacy(location, CalculateDelay(delay),
                                 std::move(user_task));
}

bool BackoffTimer::IsRunning() const {
  return timer_.IsRunning();
}

bool BackoffTimer::Stop() {
  // Reset the backoff count so that the next call to `Start` will not backoff
  // and will use the given delay.
  backoff_count_ = 0;

  return timer_.Stop();
}

///////////////////////////////////////////////////////////////////////////////

base::TimeDelta BackoffTimer::CalculateDelay(base::TimeDelta delay) {
  const bool should_backoff = backoff_count_ > 0;

  int64_t delay_in_seconds = delay.InSeconds();
  delay_in_seconds <<= backoff_count_;

  backoff_count_++;
  if (backoff_count_ > kMaxBackoffCount) {
    backoff_count_ = kMaxBackoffCount;
  }

  base::TimeDelta backoff_delay = base::Seconds(delay_in_seconds);
  if (!should_backoff) {
    // If we are not backing off, do not cap the delay.
    return backoff_delay;
  }

  if (backoff_delay > max_backoff_delay_) {
    // Cap the backoff delay.
    backoff_delay = max_backoff_delay_;
  }

  return backoff_delay;
}

}  // namespace brave_ads
