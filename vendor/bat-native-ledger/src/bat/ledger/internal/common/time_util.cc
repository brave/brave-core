/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/common/time_util.h"

#include <algorithm>
#include "brave_base/random.h"

namespace braveledger_time_util {

ledger::ActivityMonth GetCurrentMonth() {
  base::Time now = base::Time::Now();
  return GetMonth(now);
}

ledger::ActivityMonth GetMonth(const base::Time& time) {
  base::Time::Exploded exploded;
  time.LocalExplode(&exploded);
  return (ledger::ActivityMonth)exploded.month;
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
  return static_cast<uint64_t>(base::Time::Now().ToDoubleT());
}

base::TimeDelta GetRandomizedDelay(base::TimeDelta delay) {
  uint64_t seconds = brave_base::random::Geometric(delay.InSecondsF());
  return base::TimeDelta::FromSeconds(static_cast<int64_t>(seconds));
}

base::TimeDelta GetRandomizedDelayWithBackoff(
    base::TimeDelta delay,
    base::TimeDelta max_delay,
    int backoff_count) {
  delay *= 1 << std::min(backoff_count, 24);
  return GetRandomizedDelay(std::min(delay, max_delay));
}

}  // namespace braveledger_time_util
