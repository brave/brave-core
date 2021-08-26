/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/backoff_timer.h"

#include <utility>

namespace ads {

BackoffTimer::BackoffTimer() {
  max_backoff_delay_ = base::TimeDelta::FromHours(1);
}

BackoffTimer::~BackoffTimer() = default;

void BackoffTimer::set_timer_for_testing(
    std::unique_ptr<base::OneShotTimer> timer) {
  timer_.set_timer_for_testing(std::move(timer));
}

base::Time BackoffTimer::Start(const base::TimeDelta& delay,
                               base::OnceClosure user_task) {
  timer_.Stop();

  const base::TimeDelta backoff_delay = CalculateDelay(delay);
  return timer_.Start(backoff_delay, std::move(user_task));
}

base::Time BackoffTimer::StartWithPrivacy(const base::TimeDelta& delay,
                                          base::OnceClosure user_task) {
  timer_.Stop();

  const base::TimeDelta backoff_delay = CalculateDelay(delay);
  return timer_.StartWithPrivacy(backoff_delay, std::move(user_task));
}

bool BackoffTimer::IsRunning() const {
  return timer_.IsRunning();
}

void BackoffTimer::FireNow() {
  return timer_.FireNow();
}

bool BackoffTimer::Stop() {
  backoff_count_ = 0;

  return timer_.Stop();
}

void BackoffTimer::set_max_backoff_delay(const base::TimeDelta& max_delay) {
  max_backoff_delay_ = max_delay;
}

///////////////////////////////////////////////////////////////////////////////

base::TimeDelta BackoffTimer::CalculateDelay(const base::TimeDelta& delay) {
  int64_t delay_as_int64 = static_cast<int64_t>(delay.InSeconds());
  delay_as_int64 <<= backoff_count_++;

  base::TimeDelta backoff_delay = base::TimeDelta::FromSeconds(delay_as_int64);
  if (backoff_delay > max_backoff_delay_) {
    backoff_delay = max_backoff_delay_;
  }

  return backoff_delay;
}

}  // namespace ads
