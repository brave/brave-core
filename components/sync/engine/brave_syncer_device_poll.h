/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_SYNCER_DEVICE_POLL_H_
#define BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_SYNCER_DEVICE_POLL_H_

#include "base/functional/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/time/time.h"

namespace syncer {

FORWARD_DECLARE_TEST(BraveSyncerDevicePoll, ForcedPolling);

class BraveSyncerDevicePoll {
 public:
  BraveSyncerDevicePoll(const BraveSyncerDevicePoll&) = delete;
  BraveSyncerDevicePoll& operator=(const BraveSyncerDevicePoll&) = delete;
  BraveSyncerDevicePoll();

  void CheckIntervalAndPoll(base::OnceClosure poll_devices);
  base::TimeDelta SinceBegin();

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveSyncerDevicePoll, ForcedPolling);
  static base::TimeDelta GetDelayBeforeForcedPollForTesting();

  const base::TimeTicks ticks_at_begin_;
  base::TimeTicks ticks_;
};

}  // namespace syncer

#endif  // BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_SYNCER_DEVICE_POLL_H_
