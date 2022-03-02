/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_block_tracker.h"

#include "base/notreached.h"

namespace brave_wallet {

FilBlockTracker::FilBlockTracker(JsonRpcService* json_rpc_service)
    : BlockTracker(json_rpc_service) {}

void FilBlockTracker::Start(base::TimeDelta interval) {
  NOTIMPLEMENTED();
}

}  // namespace brave_wallet
