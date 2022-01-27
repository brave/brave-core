/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_block_tracker.h"

#include "base/notreached.h"

namespace brave_wallet {

SolanaBlockTracker::SolanaBlockTracker(JsonRpcService* json_rpc_service)
    : BlockTracker(json_rpc_service) {}

void SolanaBlockTracker::Start(base::TimeDelta interval) {
  NOTIMPLEMENTED();
}

}  // namespace brave_wallet
