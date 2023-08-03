/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"

#include <algorithm>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/eth_tx_meta.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/tx_state_manager.h"
#include "brave/components/brave_wallet/common/eth_address.h"

namespace brave_wallet {

EthNonceTracker::EthNonceTracker(TxStateManager* tx_state_manager,
                                 JsonRpcService* json_rpc_service)
    : NonceTracker(tx_state_manager, json_rpc_service), weak_factory_(this) {}

EthNonceTracker::~EthNonceTracker() = default;

void EthNonceTracker::GetNextNonce(const std::string& chain_id,
                                   const mojom::AccountIdPtr& from,
                                   GetNextNonceCallback callback) {
  json_rpc_service_->GetEthTransactionCount(
      chain_id, from->address,
      base::BindOnce(&EthNonceTracker::OnGetNetworkNonce,
                     weak_factory_.GetWeakPtr(), chain_id, from.Clone(),
                     std::move(callback)));
}

uint256_t EthNonceTracker::GetHighestLocallyConfirmed(
    const std::vector<std::unique_ptr<TxMeta>>& metas) {
  uint256_t highest = 0;
  for (auto& meta : metas) {
    auto* eth_meta = static_cast<EthTxMeta*>(meta.get());
    DCHECK(
        eth_meta->tx()->nonce());  // Not supposed to happen for a confirmed tx.
    highest = std::max(highest, eth_meta->tx()->nonce().value() + (uint256_t)1);
  }
  return highest;
}

uint256_t EthNonceTracker::GetHighestContinuousFrom(
    const std::vector<std::unique_ptr<TxMeta>>& metas,
    uint256_t start) {
  uint256_t highest = start;
  for (auto& meta : metas) {
    auto* eth_meta = static_cast<EthTxMeta*>(meta.get());
    DCHECK(
        eth_meta->tx()->nonce());  // Not supposed to happen for a submitted tx.
    if (eth_meta->tx()->nonce().value() == highest) {
      highest++;
    }
  }
  return highest;
}

void EthNonceTracker::OnGetNetworkNonce(const std::string& chain_id,
                                        const mojom::AccountIdPtr& from,
                                        GetNextNonceCallback callback,
                                        uint256_t network_nonce,
                                        mojom::ProviderError error,
                                        const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    std::move(callback).Run(false, network_nonce);
    return;
  }
  auto nonce = GetFinalNonce(chain_id, from, network_nonce);
  std::move(callback).Run(true, nonce);
}

}  // namespace brave_wallet
