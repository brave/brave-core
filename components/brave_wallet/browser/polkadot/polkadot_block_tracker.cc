/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_block_tracker.h"

#include "base/notimplemented.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"

namespace brave_wallet {

PolkadotBlockTracker::PolkadotBlockTracker(PolkadotSubstrateRpc& polkadot_rpc)
    : polkadot_rpc_(polkadot_rpc) {}

PolkadotBlockTracker::~PolkadotBlockTracker() = default;

void PolkadotBlockTracker::Start(const std::string& chain_id,
                                 base::TimeDelta interval) {
  auto& timer = timers_[chain_id];
  if (!timer) {
    timer = std::make_unique<base::RepeatingTimer>();
  }

  timer->Start(FROM_HERE, interval,
               base::BindRepeating(&PolkadotBlockTracker::GetLatestBlock,
                                   weak_ptr_factory_.GetWeakPtr(), chain_id));
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
  polkadot_rpc_->GetFinalizedHead(
      chain_id, base::BindOnce(&PolkadotBlockTracker::OnGetFinalizedHead,
                               weak_ptr_factory_.GetWeakPtr(), chain_id));
}

void PolkadotBlockTracker::OnGetFinalizedHead(
    std::string chain_id,
    std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> block_hash,
    std::optional<std::string>) {
  CHECK(block_hash.has_value());

  for (auto& observer : observers_) {
    observer.OnLatestBlock(chain_id, 0);
  }
}

void PolkadotBlockTracker::OnGetLatestBlock(const std::string& chain_id,
                                            uint64_t block_num,
                                            mojom::ProviderError error,
                                            const std::string& error_message) {
  NOTIMPLEMENTED_LOG_ONCE();
}

}  // namespace brave_wallet
