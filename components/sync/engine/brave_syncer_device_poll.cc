/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/engine/brave_syncer_device_poll.h"

#include <utility>

#include "base/functional/callback.h"
#include "base/time/time.h"

using base::TimeTicks;

namespace syncer {

namespace {
constexpr base::TimeDelta kDelayBeforeForcedPoll = base::Seconds(15);
}

BraveSyncerDevicePoll::BraveSyncerDevicePoll()
    : ticks_at_begin_(TimeTicks::Now()), ticks_(ticks_at_begin_) {}

void BraveSyncerDevicePoll::CheckIntervalAndPoll(
    base::OnceClosure poll_devices) {
  auto now = TimeTicks::Now();
  if (now - ticks_ > kDelayBeforeForcedPoll) {
    std::move(poll_devices).Run();
    ticks_ = now;
  }
}

/* static */
base::TimeDelta BraveSyncerDevicePoll::GetDelayBeforeForcedPollForTesting() {
  return kDelayBeforeForcedPoll;
}

base::TimeDelta BraveSyncerDevicePoll::SinceBegin() {
  return TimeTicks::Now() - ticks_at_begin_;
}

}  // namespace syncer
