/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/block_tracker.h"

#include "base/containers/map_util.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"

namespace brave_wallet {

BlockTracker::BlockTracker() = default;

BlockTracker::~BlockTracker() = default;

void BlockTracker::Stop(const std::string& chain_id) {
  auto timer = timers_.find(chain_id);
  if (timer != timers_.end()) {
    timers_.erase(timer);
  }
}

void BlockTracker::Stop() {
  timers_.clear();
}

bool BlockTracker::IsRunning(const std::string& chain_id) const {
  auto* timer = base::FindPtrOrNull(timers_, chain_id);
  if (!timer) {
    return false;
  }
  return timer->IsRunning();
}

}  // namespace brave_wallet
