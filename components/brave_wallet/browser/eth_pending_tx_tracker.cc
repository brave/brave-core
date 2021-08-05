/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_pending_tx_tracker.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

EthPendingTxTracker::EthPendingTxTracker(
    EthTxStateManager* tx_state_manager,
    EthNonceTracker* nonce_tracker,
    mojo::PendingRemote<mojom::EthJsonRpcController>
        eth_json_rpc_controller_pending)
    : tx_state_manager_(tx_state_manager),
      nonce_tracker_(nonce_tracker),
      weak_factory_(this) {
  eth_json_rpc_controller_.Bind(std::move(eth_json_rpc_controller_pending));
  DCHECK(eth_json_rpc_controller_);
  eth_json_rpc_controller_.set_disconnect_handler(base::BindOnce(
      &EthPendingTxTracker::OnConnectionError, weak_factory_.GetWeakPtr()));
}

EthPendingTxTracker::~EthPendingTxTracker() = default;

void EthPendingTxTracker::UpdatePendingTransactions() {
  base::Lock* nonce_lock = nonce_tracker_->GetLock();
  if (!nonce_lock->Try())
    return;

  auto pending_transactions = tx_state_manager_->GetTransactionsByStatus(
      EthTxStateManager::TransactionStatus::SUBMITTED, absl::nullopt);
  for (const auto& pending_transaction : pending_transactions) {
    if (IsNonceTaken(*pending_transaction)) {
      DropTransaction(pending_transaction.get());
      continue;
    }
    std::string id = pending_transaction->id;
    if (eth_json_rpc_controller_) {
      eth_json_rpc_controller_->GetTransactionReceipt(
          pending_transaction->tx_hash,
          base::BindOnce(&EthPendingTxTracker::OnGetTxReceipt,
                         weak_factory_.GetWeakPtr(), std::move(id)));
    }
  }

  nonce_lock->Release();
}

void EthPendingTxTracker::ResubmitPendingTransactions() {
  if (!eth_json_rpc_controller_) {
    LOG(ERROR) << "Could not resubmit pending transaction because "
                  "eth_json_rpc_controller_ is not available";
    return;
  }
  // TODO(darkdh): limit the rate of tx publishing
  auto pending_transactions = tx_state_manager_->GetTransactionsByStatus(
      EthTxStateManager::TransactionStatus::SUBMITTED, absl::nullopt);
  for (const auto& pending_transaction : pending_transactions) {
    if (!pending_transaction->tx->IsSigned()) {
      continue;
    }

    std::string signed_transaction;
    pending_transaction->tx->GetSignedTransaction(base::BindOnce(
        &EthPendingTxTracker::OnGetSignedTransactionResubmitPending,
        weak_factory_.GetWeakPtr()));
  }
}

void EthPendingTxTracker::OnGetSignedTransactionResubmitPending(
    bool status,
    const std::string& signed_transaction) {
  if (status) {
    eth_json_rpc_controller_->SendRawTransaction(
        signed_transaction,
        base::BindOnce(&EthPendingTxTracker::OnSendRawTransaction,
                       weak_factory_.GetWeakPtr()));
  }
}

void EthPendingTxTracker::OnGetTxReceipt(std::string id,
                                         bool status,
                                         mojom::TransactionReceiptPtr receipt) {
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
  if (receipt->status == true) {
    meta->tx_receipt = std::move(receipt);
    meta->status = EthTxStateManager::TransactionStatus::CONFIRMED;
    meta->confirmed_time = base::Time::Now();
    tx_state_manager_->AddOrUpdateTx(*meta);
  } else if (ShouldTxDropped(*meta)) {
    DropTransaction(meta.get());
  }

  nonce_lock->Release();
}

void EthPendingTxTracker::OnGetNetworkNonce(std::string address,
                                            bool status,
                                            const std::string& result) {
  if (!status)
    return;
  network_nonce_map_[address] = result;
}

void EthPendingTxTracker::OnSendRawTransaction(bool status,
                                               const std::string& tx_hash) {}

bool EthPendingTxTracker::IsNonceTaken(const EthTxStateManager::TxMeta& meta) {
  auto confirmed_transactions = tx_state_manager_->GetTransactionsByStatus(
      EthTxStateManager::TransactionStatus::CONFIRMED, absl::nullopt);
  for (const auto& confirmed_transaction : confirmed_transactions) {
    if (confirmed_transaction->tx->nonce() == meta.tx->nonce() &&
        confirmed_transaction->id != meta.id)
      return true;
  }
  return false;
}

bool EthPendingTxTracker::ShouldTxDropped(
    const EthTxStateManager::TxMeta& meta) {
  if (!eth_json_rpc_controller_) {
    LOG(ERROR) << "Could not check if tx should be dropped because "
                  "eth_json_rpc_controller_ is not available";
    return false;
  }
  const std::string hex_address = meta.from.ToHex();
  if (network_nonce_map_.find(hex_address) == network_nonce_map_.end()) {
    eth_json_rpc_controller_->GetTransactionCount(
        hex_address,
        base::BindOnce(&EthPendingTxTracker::OnGetNetworkNonce,
                       weak_factory_.GetWeakPtr(), std::move(hex_address)));
  } else {
    std::string network_nonce = network_nonce_map_[hex_address];

    uint256_t network_nonce_uint;
    if (!HexValueToUint256(network_nonce, &network_nonce_uint))
      return false;

    uint256_t meta_tx_nonce;
    if (!HexValueToUint256(meta.tx->nonce(), &meta_tx_nonce))
      return false;

    network_nonce_map_.erase(hex_address);
    if (meta_tx_nonce < network_nonce_uint)
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

void EthPendingTxTracker::OnConnectionError() {
  eth_json_rpc_controller_.reset();
}

}  // namespace brave_wallet
