/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/timer/timer.h"

#include <algorithm>
#include <cstdint>
#include <utility>

#include "base/time/time.h"
#include "brave_base/random.h"

namespace brave_ads {

Timer::Timer() = default;

Timer::~Timer() {
  Stop();
}

base::Time Timer::Start(const base::Location& location,
                        const base::TimeDelta delay,
                        base::OnceClosure user_task) {
  Stop();

  const base::Time fire_at = base::Time::Now() + delay;
  timer_.Start(location, fire_at, std::move(user_task));
  return fire_at;
}

base::Time Timer::StartWithPrivacy(const base::Location& location,
                                   const base::TimeDelta delay,
                                   base::OnceClosure user_task) {
  const int64_t rand_delay_in_seconds = std::max(
      int64_t{1},
      static_cast<int64_t>(brave_base::random::Geometric(delay.InSecondsF())));

  return Start(location, base::Seconds(rand_delay_in_seconds),
               std::move(user_task));
}

bool Timer::IsRunning() const {
  return timer_.IsRunning();
}

bool Timer::Stop() {
  if (!IsRunning()) {
    return false;
  }

  timer_.Stop();

  return true;
}

}  // namespace brave_ads
