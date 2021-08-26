/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_pending_tx_tracker.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

EthPendingTxTracker::EthPendingTxTracker(EthTxStateManager* tx_state_manager,
                                         EthJsonRpcController* rpc_controller,
                                         EthNonceTracker* nonce_tracker)
    : tx_state_manager_(tx_state_manager),
      rpc_controller_(rpc_controller),
      nonce_tracker_(nonce_tracker),
      weak_factory_(this) {}
EthPendingTxTracker::~EthPendingTxTracker() = default;

void EthPendingTxTracker::UpdatePendingTransactions() {
  base::Lock* nonce_lock = nonce_tracker_->GetLock();
  if (!nonce_lock->Try())
    return;

  auto pending_transactions = tx_state_manager_->GetTransactionsByStatus(
      mojom::TransactionStatus::Submitted, absl::nullopt);
  for (const auto& pending_transaction : pending_transactions) {
    if (IsNonceTaken(*pending_transaction)) {
      DropTransaction(pending_transaction.get());
      continue;
    }
    std::string id = pending_transaction->id;
    rpc_controller_->GetTransactionReceipt(
        pending_transaction->tx_hash,
        base::BindOnce(&EthPendingTxTracker::OnGetTxReceipt,
                       weak_factory_.GetWeakPtr(), std::move(id)));
  }

  nonce_lock->Release();
}

void EthPendingTxTracker::ResubmitPendingTransactions() {
  // TODO(darkdh): limit the rate of tx publishing
  auto pending_transactions = tx_state_manager_->GetTransactionsByStatus(
      mojom::TransactionStatus::Submitted, absl::nullopt);
  for (const auto& pending_transaction : pending_transactions) {
    if (!pending_transaction->tx->IsSigned()) {
      continue;
    }
    rpc_controller_->SendRawTransaction(
        pending_transaction->tx->GetSignedTransaction(),
        base::BindOnce(&EthPendingTxTracker::OnSendRawTransaction,
                       weak_factory_.GetWeakPtr()));
  }
}

void EthPendingTxTracker::OnGetTxReceipt(std::string id,
                                         bool status,
                                         TransactionReceipt receipt) {
  if (!status)
    return;
  base::Lock* nonce_lock = nonce_tracker_->GetLock();
  if (!nonce_lock->Try())
    return;

  std::unique_ptr<EthTxStateManager::TxMeta> meta =
      tx_state_manager_->GetTx(id);
  if (!meta) {
    nonce_lock->Release();
    return;
  }
  if (receipt.status) {
    meta->tx_receipt = receipt;
    meta->status = mojom::TransactionStatus::Confirmed;
    meta->confirmed_time = base::Time::Now();
    tx_state_manager_->AddOrUpdateTx(*meta);
  } else if (ShouldTxDropped(*meta)) {
    DropTransaction(meta.get());
  }

  nonce_lock->Release();
}

void EthPendingTxTracker::OnGetNetworkNonce(std::string address,
                                            bool status,
                                            uint256_t result) {
  if (!status)
    return;
  network_nonce_map_[address] = result;
}

void EthPendingTxTracker::OnSendRawTransaction(bool status,
                                               const std::string& tx_hash) {}

bool EthPendingTxTracker::IsNonceTaken(const EthTxStateManager::TxMeta& meta) {
  auto confirmed_transactions = tx_state_manager_->GetTransactionsByStatus(
      mojom::TransactionStatus::Confirmed, absl::nullopt);
  for (const auto& confirmed_transaction : confirmed_transactions) {
    if (confirmed_transaction->tx->nonce() == meta.tx->nonce() &&
        confirmed_transaction->id != meta.id)
      return true;
  }
  return false;
}

bool EthPendingTxTracker::ShouldTxDropped(
    const EthTxStateManager::TxMeta& meta) {
  const std::string hex_address = meta.from.ToHex();
  if (network_nonce_map_.find(hex_address) == network_nonce_map_.end()) {
    rpc_controller_->GetTransactionCount(
        hex_address,
        base::BindOnce(&EthPendingTxTracker::OnGetNetworkNonce,
                       weak_factory_.GetWeakPtr(), std::move(hex_address)));
  } else {
    uint256_t network_nonce = network_nonce_map_[hex_address];
    network_nonce_map_.erase(hex_address);
    if (meta.tx->nonce() < network_nonce)
      return true;
  }

  const std::string tx_hash = meta.tx_hash;
  if (dropped_blocks_counter_.find(tx_hash) == dropped_blocks_counter_.end()) {
    dropped_blocks_counter_[tx_hash] = 0;
  }
  if (dropped_blocks_counter_[tx_hash] >= 3) {
    dropped_blocks_counter_.erase(tx_hash);
    return true;
  }

  dropped_blocks_counter_[tx_hash] = dropped_blocks_counter_[tx_hash] + 1;

  return false;
}

void EthPendingTxTracker::DropTransaction(EthTxStateManager::TxMeta* meta) {
  if (!meta)
    return;
  tx_state_manager_->DeleteTx(meta->id);
}

}  // namespace brave_wallet
