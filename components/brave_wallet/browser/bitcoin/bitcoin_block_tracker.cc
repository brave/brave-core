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
    const std::string& network_id,
    JsonRpcService* json_rpc_service,
    BitcoinWalletService* bitcoin_wallet_service)
    : BlockTracker(json_rpc_service),
      network_id_(network_id),
      bitcoin_wallet_service_(bitcoin_wallet_service) {}

BitcoinBlockTracker::~BitcoinBlockTracker() = default;

void BitcoinBlockTracker::Start(base::TimeDelta interval) {
  timer_.Start(FROM_HERE, interval,
               base::BindRepeating(&BitcoinBlockTracker::GetBlockHeight,
                                   weak_ptr_factory_.GetWeakPtr()));
}

void BitcoinBlockTracker::GetBlockHeight() {
  bitcoin_wallet_service_->bitcoin_rpc()->GetChainHeight(
      network_id_, base::BindOnce(&BitcoinBlockTracker::OnGetBlockHeight,
                                  weak_ptr_factory_.GetWeakPtr()));
}

void BitcoinBlockTracker::OnGetBlockHeight(
    base::expected<uint32_t, std::string> latest_height) {
  if (!latest_height.has_value()) {
    return;
  }
  if (latest_height_ == latest_height) {
    return;
  }
  latest_height_ = latest_height.value();
  for (auto& observer : observers_) {
    observer.OnLatestHeightUpdated(network_id_, latest_height_);
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
