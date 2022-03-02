/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/block_tracker.h"

#include "brave/components/brave_wallet/browser/json_rpc_service.h"

namespace brave_wallet {

BlockTracker::BlockTracker(JsonRpcService* json_rpc_service)
    : json_rpc_service_(json_rpc_service) {
  DCHECK(json_rpc_service_);
}

void BlockTracker::Stop() {
  timer_.Stop();
}

bool BlockTracker::IsRunning() const {
  return timer_.IsRunning();
}

}  // namespace brave_wallet
