/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"

namespace brave_wallet {

namespace {

uint256_t GetHighestLocallyConfirmed(
    const std::vector<std::unique_ptr<EthTxStateManager::TxMeta>>& metas) {
  uint256_t highest = 0;
  for (auto& meta : metas) {
    uint256_t nonce_uint;
    if (HexValueToUint256(meta->tx->nonce(), &nonce_uint)) {
      highest = std::max(highest, nonce_uint);
    }
  }
  return ++highest;
}

uint256_t GetHighestContinuousFrom(
    const std::vector<std::unique_ptr<EthTxStateManager::TxMeta>>& metas,
    uint256_t start) {
  uint256_t highest = start;
  for (auto& meta : metas) {
    uint256_t nonce_uint;
    if (HexValueToUint256(meta->tx->nonce(), &nonce_uint)) {
      if (nonce_uint == highest) {
        highest++;
      }
    }
  }
  return highest;
}

}  // namespace

EthNonceTracker::EthNonceTracker(
    EthTxStateManager* tx_state_manager,
    mojo::PendingRemote<mojom::EthJsonRpcController>
        eth_json_rpc_controller_pending)
    : tx_state_manager_(tx_state_manager), weak_factory_(this) {
  eth_json_rpc_controller_.Bind(std::move(eth_json_rpc_controller_pending));
  DCHECK(eth_json_rpc_controller_);
  eth_json_rpc_controller_.set_disconnect_handler(base::BindOnce(
      &EthNonceTracker::OnConnectionError, weak_factory_.GetWeakPtr()));
}

EthNonceTracker::~EthNonceTracker() = default;

void EthNonceTracker::GetNextNonce(const EthAddress& from,
                                   GetNextNonceCallback callback) {
  const std::string hex_address = from.ToHex();
  if (!eth_json_rpc_controller_) {
    std::move(callback).Run(false, "");
    return;
  }

  eth_json_rpc_controller_->GetTransactionCount(
      hex_address, base::BindOnce(&EthNonceTracker::OnGetNetworkNonce,
                                  weak_factory_.GetWeakPtr(), EthAddress(from),
                                  std::move(callback)));
}

void EthNonceTracker::OnGetNetworkNonce(EthAddress from,
                                        GetNextNonceCallback callback,
                                        bool status,
                                        const std::string& network_nonce) {
  if (!nonce_lock_.Try()) {
    std::move(callback).Run(false, network_nonce);
    return;
  }
  auto confirmed_transactions = tx_state_manager_->GetTransactionsByStatus(
      EthTxStateManager::TransactionStatus::CONFIRMED, from);
  uint256_t local_highest = GetHighestLocallyConfirmed(confirmed_transactions);

  uint256_t network_nonce_uint;
  if (!HexValueToUint256(network_nonce, &network_nonce_uint)) {
    nonce_lock_.Release();
    std::move(callback).Run(false, "");
    return;
  }

  uint256_t highest_confirmed = std::max(network_nonce_uint, local_highest);

  auto pending_transactions = tx_state_manager_->GetTransactionsByStatus(
      EthTxStateManager::TransactionStatus::SUBMITTED, from);

  uint256_t highest_continuous_from =
      GetHighestContinuousFrom(pending_transactions, highest_confirmed);

  uint256_t nonce = std::max(network_nonce_uint, highest_continuous_from);

  nonce_lock_.Release();
  std::move(callback).Run(true, Uint256ValueToHex(nonce));
}

void EthNonceTracker::OnConnectionError() {
  eth_json_rpc_controller_.reset();
}

}  // namespace brave_wallet
