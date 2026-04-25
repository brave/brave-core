/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_block_tracker.h"

#include "base/notimplemented.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"

namespace brave_wallet {

PolkadotBlockTracker::PolkadotBlockTracker() : weak_factory_(this) {}

PolkadotBlockTracker::~PolkadotBlockTracker() = default;

void PolkadotBlockTracker::Start(const std::string& chain_id,
                                 base::TimeDelta interval) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void PolkadotBlockTracker::Stop(const std::string& chain_id) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void PolkadotBlockTracker::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void PolkadotBlockTracker::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void PolkadotBlockTracker::GetLatestBlock(const std::string& chain_id) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void PolkadotBlockTracker::OnGetLatestBlock(const std::string& chain_id,
                                            uint64_t block_num,
                                            mojom::ProviderError error,
                                            const std::string& error_message) {
  NOTIMPLEMENTED_LOG_ONCE();
}

}  // namespace brave_wallet
