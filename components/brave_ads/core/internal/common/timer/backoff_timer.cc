/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/timer/backoff_timer.h"

#include <algorithm>
#include <utility>

#include "base/check.h"

namespace brave_ads {

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

  backoff_delay_ = CalculateDelay(delay);
  CHECK(backoff_delay_);

  return timer_.StartWithPrivacy(location, *backoff_delay_,
                                 std::move(user_task));
}

bool BackoffTimer::IsRunning() const {
  return timer_.IsRunning();
}

bool BackoffTimer::Stop() {
  // Reset the backoff so the next Start does not double the delay.
  backoff_delay_.reset();
  return timer_.Stop();
}

///////////////////////////////////////////////////////////////////////////////

base::TimeDelta BackoffTimer::CalculateDelay(base::TimeDelta delay) const {
  return backoff_delay_ ? std::min(*backoff_delay_ * 2, max_backoff_delay_)
                        : delay;
}

}  // namespace brave_ads
