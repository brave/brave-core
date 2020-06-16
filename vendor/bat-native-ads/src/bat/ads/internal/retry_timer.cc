/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ads/internal/retry_timer.h"

namespace ads {

RetryTimer::RetryTimer() = default;

RetryTimer::~RetryTimer() = default;

void RetryTimer::set_timer_for_testing(
    std::unique_ptr<base::OneShotTimer> timer) {
  timer_.set_timer_for_testing(std::move(timer));
}

base::Time RetryTimer::Start(
    const uint64_t delay,
    base::OnceClosure user_task) {
  timer_.Stop();

  const base::Time time = timer_.StartWithPrivacy(delay, std::move(user_task));

  return time;
}

base::Time RetryTimer::StartWithBackoff(
    const uint64_t delay,
    base::OnceClosure user_task) {
  uint64_t backoff_delay = delay;

  backoff_delay <<= backoff_count_++;
  if (backoff_delay > max_backoff_delay_) {
    backoff_delay = max_backoff_delay_;
  }

  return Start(backoff_delay, std::move(user_task));
}

bool RetryTimer::IsRunning() const {
  return timer_.IsRunning();
}

void RetryTimer::Stop() {
  timer_.Stop();

  backoff_count_ = 0;
}

void RetryTimer::set_max_backoff_delay(
    const uint64_t max_delay) {
  max_backoff_delay_ = max_delay;
}

}  // namespace ads
