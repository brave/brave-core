/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/timer/timer.h"

#include <optional>
#include <utility>

#include "base/check_is_test.h"
#include "base/debug/crash_logging.h"
#include "base/debug/dump_without_crashing.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/common/random/random_util.h"

namespace brave_ads {

namespace {
std::optional<base::TimeDelta> g_timer_delay_for_testing;
}  // namespace

Timer::Timer() = default;

Timer::~Timer() {
  Stop();
}

base::Time Timer::Start(const base::Location& location,
                        const base::TimeDelta delay,
                        base::OnceClosure user_task) {
  Stop();

  const base::Time fire_at =
      base::Time::Now() + g_timer_delay_for_testing.value_or(delay);
  timer_.Start(location, fire_at, std::move(user_task));
  return fire_at;
}

base::Time Timer::StartWithPrivacy(const base::Location& location,
                                   const base::TimeDelta delay,
                                   base::OnceClosure user_task) {
  base::TimeDelta rand_delay = RandTimeDelta(delay);
  if (rand_delay.is_negative()) {
    // TODO(https://github.com/brave/brave-browser/issues/32066):
    // Detect potential defects using `DumpWithoutCrashing`.
    SCOPED_CRASH_KEY_NUMBER("Issue32066", "rand_delay",
                            rand_delay.InMicroseconds());
    SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                              "Invalid random timer delay");
    base::debug::DumpWithoutCrashing();

    rand_delay = base::Seconds(1);
  }

  return Start(location, rand_delay, std::move(user_task));
}

bool Timer::IsRunning() const {
  return timer_.IsRunning();
}

bool Timer::Stop() {
  const bool was_running = IsRunning();
  timer_.Stop();
  return was_running;
}

ScopedTimerDelaySetterForTesting::ScopedTimerDelaySetterForTesting(
    const base::TimeDelta delay) {
  CHECK_IS_TEST();

  g_timer_delay_for_testing = delay;
}

ScopedTimerDelaySetterForTesting::~ScopedTimerDelaySetterForTesting() {
  g_timer_delay_for_testing = std::nullopt;
}

}  // namespace brave_ads
