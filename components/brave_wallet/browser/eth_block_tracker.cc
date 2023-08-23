/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_block_tracker.h"

#include <memory>
#include <utility>

#include "base/containers/contains.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"

namespace brave_wallet {

EthBlockTracker::EthBlockTracker(JsonRpcService* json_rpc_service)
    : json_rpc_service_(json_rpc_service), weak_factory_(this) {}

EthBlockTracker::~EthBlockTracker() = default;

void EthBlockTracker::Start(const std::string& chain_id,
                            base::TimeDelta interval) {
  if (!base::Contains(timers_, chain_id)) {
    timers_[chain_id] = std::make_unique<base::RepeatingTimer>();
  }
  timers_[chain_id]->Start(
      FROM_HERE, interval,
      base::BindRepeating(&EthBlockTracker::GetBlockNumber,
                          weak_factory_.GetWeakPtr(), chain_id));
}

void EthBlockTracker::AddObserver(EthBlockTracker::Observer* observer) {
  observers_.AddObserver(observer);
}

void EthBlockTracker::RemoveObserver(EthBlockTracker::Observer* observer) {
  observers_.RemoveObserver(observer);
}

uint256_t EthBlockTracker::GetCurrentBlock(const std::string& chain_id) const {
  if (!base::Contains(current_block_map_, chain_id)) {
    return 0;
  }
  return current_block_map_.at(chain_id);
}

void EthBlockTracker::CheckForLatestBlock(
    const std::string& chain_id,
    base::OnceCallback<void(uint256_t block_num,
                            mojom::ProviderError error,
                            const std::string& error_message)> callback) {
  SendGetBlockNumber(chain_id, std::move(callback));
}

void EthBlockTracker::SendGetBlockNumber(
    const std::string& chain_id,
    base::OnceCallback<void(uint256_t block_num,
                            mojom::ProviderError error,
                            const std::string& error_message)> callback) {
  json_rpc_service_->GetBlockNumber(chain_id, std::move(callback));
}

void EthBlockTracker::GetBlockNumber(const std::string& chain_id) {
  json_rpc_service_->GetBlockNumber(
      chain_id, base::BindOnce(&EthBlockTracker::OnGetBlockNumber,
                               weak_factory_.GetWeakPtr(), chain_id));
}

void EthBlockTracker::OnGetBlockNumber(const std::string& chain_id,
                                       uint256_t block_num,
                                       mojom::ProviderError error,
                                       const std::string& error_message) {
  if (error == mojom::ProviderError::kSuccess) {
    if (GetCurrentBlock(chain_id) != block_num) {
      current_block_map_[chain_id] = block_num;
      for (auto& observer : observers_) {
        observer.OnNewBlock(chain_id, block_num);
      }
    }
    for (auto& observer : observers_) {
      observer.OnLatestBlock(chain_id, block_num);
    }

  } else {
    LOG(ERROR) << "GetBlockNumber failed";
  }
}

}  // namespace brave_wallet
