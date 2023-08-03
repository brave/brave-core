/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_nonce_tracker.h"

#include <algorithm>
#include <utility>

#include "brave/components/brave_wallet/browser/fil_tx_meta.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/tx_state_manager.h"

namespace brave_wallet {

FilNonceTracker::FilNonceTracker(TxStateManager* tx_state_manager,
                                 JsonRpcService* json_rpc_service)
    : NonceTracker(tx_state_manager, json_rpc_service), weak_factory_(this) {}

FilNonceTracker::~FilNonceTracker() = default;

void FilNonceTracker::GetNextNonce(const std::string& chain_id,
                                   const mojom::AccountIdPtr& from,
                                   GetNextNonceCallback callback) {
  json_rpc_service_->GetFilTransactionCount(
      chain_id, from->address,
      base::BindOnce(&FilNonceTracker::OnGetNetworkNonce,
                     weak_factory_.GetWeakPtr(), chain_id, from.Clone(),
                     std::move(callback)));
}

uint256_t FilNonceTracker::GetHighestLocallyConfirmed(
    const std::vector<std::unique_ptr<TxMeta>>& metas) {
  uint64_t highest = 0;
  for (auto& meta : metas) {
    auto* fil_meta = static_cast<FilTxMeta*>(meta.get());
    DCHECK(
        fil_meta->tx()->nonce());  // Not supposed to happen for a confirmed tx.
    highest = std::max(highest, fil_meta->tx()->nonce().value() + (uint64_t)1);
  }
  return static_cast<uint256_t>(highest);
}

uint256_t FilNonceTracker::GetHighestContinuousFrom(
    const std::vector<std::unique_ptr<TxMeta>>& metas,
    uint256_t start) {
  bool valid_range = start <= static_cast<uint256_t>(UINT64_MAX);
  DCHECK(valid_range);
  uint64_t highest = static_cast<uint64_t>(start);
  for (auto& meta : metas) {
    auto* fil_meta = static_cast<FilTxMeta*>(meta.get());
    DCHECK(
        fil_meta->tx()->nonce());  // Not supposed to happen for a submitted tx.
    if (fil_meta->tx()->nonce().value() == highest) {
      highest++;
    }
  }
  return static_cast<uint256_t>(highest);
}

void FilNonceTracker::OnGetNetworkNonce(const std::string& chain_id,
                                        const mojom::AccountIdPtr& from,
                                        GetNextNonceCallback callback,
                                        uint256_t network_nonce,
                                        mojom::FilecoinProviderError error,
                                        const std::string& error_message) {
  if (error != mojom::FilecoinProviderError::kSuccess) {
    std::move(callback).Run(false, network_nonce);
    return;
  }
  auto nonce = GetFinalNonce(chain_id, from, network_nonce);
  std::move(callback).Run(true, nonce);
}

}  // namespace brave_wallet
