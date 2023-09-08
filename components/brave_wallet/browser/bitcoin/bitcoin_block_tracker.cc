/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_block_tracker.h"

#include <memory>
#include <utility>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_rpc.h"

namespace brave_wallet {

BitcoinBlockTracker::BitcoinBlockTracker(bitcoin_rpc::BitcoinRpc* bitcoin_rpc)
    : bitcoin_rpc_(bitcoin_rpc) {}

BitcoinBlockTracker::~BitcoinBlockTracker() = default;

void BitcoinBlockTracker::Start(const std::string& chain_id,
                                base::TimeDelta interval) {
  if (!base::Contains(timers_, chain_id)) {
    timers_[chain_id] = std::make_unique<base::RepeatingTimer>();
  }

  timers_[chain_id]->Start(
      FROM_HERE, interval,
      base::BindRepeating(&BitcoinBlockTracker::GetBlockHeight,
                          weak_ptr_factory_.GetWeakPtr(), chain_id));
}

void BitcoinBlockTracker::GetBlockHeight(const std::string& chain_id) {
  bitcoin_rpc_->GetChainHeight(
      chain_id, base::BindOnce(&BitcoinBlockTracker::OnGetBlockHeight,
                               weak_ptr_factory_.GetWeakPtr(), chain_id));
}

absl::optional<uint32_t> BitcoinBlockTracker::GetLatestHeight(
    const std::string& chain_id) const {
  if (!base::Contains(latest_height_map_, chain_id)) {
    return absl::nullopt;
  }
  return latest_height_map_.at(chain_id);
}

void BitcoinBlockTracker::OnGetBlockHeight(
    const std::string& chain_id,
    base::expected<uint32_t, std::string> latest_height) {
  if (!latest_height.has_value()) {
    return;
  }
  auto cur_latest_height = GetLatestHeight(chain_id);
  if (cur_latest_height && cur_latest_height.value() == latest_height) {
    return;
  }
  latest_height_map_[chain_id] = latest_height.value();
  for (auto& observer : observers_) {
    observer.OnLatestHeightUpdated(chain_id, latest_height.value());
  }
}

void BitcoinBlockTracker::AddObserver(BitcoinBlockTracker::Observer* observer) {
  observers_.AddObserver(observer);
}

void BitcoinBlockTracker::RemoveObserver(
    BitcoinBlockTracker::Observer* observer) {
  observers_.RemoveObserver(observer);
}

}  // namespace brave_wallet
