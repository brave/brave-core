/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/common/time_util.h"

#include <algorithm>

#include "brave/vendor/brave_base/random.h"

namespace brave_rewards::internal::util {

mojom::ActivityMonth GetCurrentMonth() {
  base::Time now = base::Time::Now();
  return GetMonth(now);
}

mojom::ActivityMonth GetMonth(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return (mojom::ActivityMonth)exploded.month;
}

uint32_t GetCurrentYear() {
  base::Time now = base::Time::Now();
  return GetYear(now);
}

uint32_t GetYear(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return exploded.year;
}

uint64_t GetCurrentTimeStamp() {
  return static_cast<uint64_t>(base::Time::Now().InSecondsFSinceUnixEpoch());
}

base::TimeDelta GetRandomizedDelay(base::TimeDelta delay) {
  uint64_t seconds = brave_base::random::Geometric(delay.InSecondsF());
  return base::Seconds(static_cast<int64_t>(seconds));
}

base::TimeDelta GetRandomizedDelayWithBackoff(base::TimeDelta delay,
                                              base::TimeDelta max_delay,
                                              int backoff_count) {
  delay *= 1 << std::min(backoff_count, 24);
  return GetRandomizedDelay(std::min(delay, max_delay));
}

}  // namespace brave_rewards::internal::util
