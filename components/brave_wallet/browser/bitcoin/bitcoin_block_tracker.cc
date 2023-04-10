/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_block_tracker.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"

// TOOD(apaymyshev): tests for this class
namespace brave_wallet {

BitcoinBlockTracker::BitcoinBlockTracker(
    JsonRpcService* json_rpc_service,
    BitcoinWalletService* bitcoin_wallet_service)
    : BlockTracker(json_rpc_service),
      bitcoin_wallet_service_(bitcoin_wallet_service) {}

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
  bitcoin_wallet_service_->bitcoin_rpc()->GetChainHeight(
      chain_id, base::BindOnce(&BitcoinBlockTracker::OnGetBlockHeight,
                               weak_ptr_factory_.GetWeakPtr(), chain_id));
}

void BitcoinBlockTracker::OnGetBlockHeight(
    const std::string& chain_id,
    base::expected<uint32_t, std::string> latest_height) {
  if (!latest_height.has_value()) {
    return;
  }
  if (latest_height_ == latest_height) {
    return;
  }
  latest_height_ = latest_height.value();
  for (auto& observer : observers_) {
    observer.OnLatestHeightUpdated(chain_id, latest_height_);
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
