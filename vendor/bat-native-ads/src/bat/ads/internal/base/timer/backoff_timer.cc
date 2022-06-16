/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/timer/backoff_timer.h"

#include <utility>

#include "base/location.h"
#include "base/timer/timer.h"

namespace ads {

BackoffTimer::BackoffTimer() = default;

BackoffTimer::~BackoffTimer() = default;

base::Time BackoffTimer::Start(const base::Location& location,
                               const base::TimeDelta delay,
                               base::OnceClosure user_task) {
  timer_.Stop();

  return timer_.Start(location, CalculateDelay(delay), std::move(user_task));
}

base::Time BackoffTimer::StartWithPrivacy(const base::Location& location,
                                          const base::TimeDelta delay,
                                          base::OnceClosure user_task) {
  timer_.Stop();

  const base::TimeDelta backoff_delay = CalculateDelay(delay);
  return timer_.StartWithPrivacy(location, backoff_delay, std::move(user_task));
}

bool BackoffTimer::IsRunning() const {
  return timer_.IsRunning();
}

bool BackoffTimer::Stop() {
  backoff_count_ = 0;

  return timer_.Stop();
}

///////////////////////////////////////////////////////////////////////////////

base::TimeDelta BackoffTimer::CalculateDelay(const base::TimeDelta delay) {
  int64_t delay_in_seconds = delay.InSeconds();
  delay_in_seconds <<= backoff_count_++;

  base::TimeDelta backoff_delay = base::Seconds(delay_in_seconds);
  if (backoff_delay > max_backoff_delay_) {
    backoff_delay = max_backoff_delay_;
  }

  return backoff_delay;
}

}  // namespace ads
