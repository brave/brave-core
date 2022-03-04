/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/eth_tx_meta.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/nonce_tracker.h"
#include "brave/components/brave_wallet/browser/tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/eth_address.h"

namespace brave_wallet {

EthNonceTracker::EthNonceTracker(TxStateManager* tx_state_manager,
                                 JsonRpcService* json_rpc_service)
    : NonceTracker(tx_state_manager, json_rpc_service), weak_factory_(this) {}

EthNonceTracker::~EthNonceTracker() = default;

void EthNonceTracker::GetNextNonce(const std::string& from,
                                   GetNextNonceCallback callback) {
  GetJsonRpcService()->GetEthTransactionCount(
      from,
      base::BindOnce(&EthNonceTracker::OnEthGetNetworkNonce,
                     weak_factory_.GetWeakPtr(), from, std::move(callback)));
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
    if (eth_meta->tx()->nonce().value() == highest)
      highest++;
  }
  return highest;
}

void EthNonceTracker::OnEthGetNetworkNonce(const std::string& from,
                                           GetNextNonceCallback callback,
                                           uint256_t result,
                                           mojom::ProviderError error,
                                           const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    std::move(callback).Run(false, result);
    return;
  }
  auto nonce =
      GetFinalNonce(EthAddress::FromHex(from).ToChecksumAddress(), result);
  std::move(callback).Run(nonce.has_value(), nonce.has_value() ? *nonce : 0);
}

}  // namespace brave_wallet
