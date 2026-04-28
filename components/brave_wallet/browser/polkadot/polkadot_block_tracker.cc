/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_block_tracker.h"

#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"
#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

PolkadotBlockTracker::PolkadotBlockTracker(PolkadotSubstrateRpc& polkadot_rpc)
    : polkadot_rpc_(polkadot_rpc) {}

PolkadotBlockTracker::~PolkadotBlockTracker() = default;

void PolkadotBlockTracker::Start(const std::string& chain_id,
                                 base::TimeDelta interval) {
  CHECK(IsPolkadotNetwork(chain_id));

  auto& timer = timers_[chain_id];
  if (!timer) {
    timer = std::make_unique<base::RepeatingTimer>();
  }

  timer->Start(FROM_HERE, interval,
               base::BindRepeating(&PolkadotBlockTracker::GetLatestBlock,
                                   weak_ptr_factory_.GetWeakPtr(), chain_id));
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
    const std::string& chain_id,
    std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> block_hash,
    std::optional<std::string> err_str) {
  if (err_str || !block_hash) {
    return;
  }

  auto& latest_hash = latest_block_hashes_map_[chain_id];
  if (block_hash == latest_hash) {
    return;
  }

  latest_hash = block_hash.value();
  polkadot_rpc_->GetBlockHeader(
      chain_id, latest_hash,
      base::BindOnce(&PolkadotBlockTracker::OnGetFinalizedBlockHeader,
                     weak_ptr_factory_.GetWeakPtr(), chain_id));
}

void PolkadotBlockTracker::OnGetFinalizedBlockHeader(
    const std::string& chain_id,
    std::optional<PolkadotBlockHeader> block_header,
    std::optional<std::string> err_str) {
  if (err_str || !block_header) {
    return;
  }

  observers_.Notify(&Observer::OnLatestBlock, chain_id,
                    block_header->block_number);
}

}  // namespace brave_wallet
