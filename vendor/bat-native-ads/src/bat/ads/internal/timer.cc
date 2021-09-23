/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/timer.h"

#include <cstdint>
#include <utility>

#include "base/check.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave_base/random.h"

namespace ads {

Timer::Timer() : timer_(std::make_unique<base::OneShotTimer>()) {
  DCHECK(timer_);
}

Timer::~Timer() {
  Stop();
}

void Timer::SetTimerForTesting(std::unique_ptr<base::OneShotTimer> timer) {
  timer_ = std::move(timer);
}

base::Time Timer::Start(const base::TimeDelta& delay,
                        base::OnceClosure user_task) {
  Stop();

  timer_->Start(FROM_HERE, delay, std::move(user_task));

  const base::Time time = base::Time::Now() + delay;

  return time;
}

base::Time Timer::StartWithPrivacy(const base::TimeDelta& delay,
                                   base::OnceClosure user_task) {
  const int64_t delay_in_seconds = delay.InSeconds();

  const int64_t rand_delay_in_seconds =
      static_cast<int64_t>(brave_base::random::Geometric(delay_in_seconds));

  return Start(base::TimeDelta::FromSeconds(rand_delay_in_seconds),
               std::move(user_task));
}

bool Timer::IsRunning() const {
  return timer_->IsRunning();
}

void Timer::FireNow() {
  return timer_->FireNow();
}

bool Timer::Stop() {
  if (!IsRunning()) {
    return false;
  }

  timer_->Stop();

  return true;
}

}  // namespace ads
