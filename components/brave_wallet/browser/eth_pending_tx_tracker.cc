/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_pending_tx_tracker.h"

#include <memory>
#include <utility>

#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "brave/components/brave_wallet/browser/eth_tx_meta.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/nonce_tracker.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

EthPendingTxTracker::EthPendingTxTracker(EthTxStateManager* tx_state_manager,
                                         JsonRpcService* json_rpc_service,
                                         NonceTracker* nonce_tracker)
    : tx_state_manager_(tx_state_manager),
      json_rpc_service_(json_rpc_service),
      nonce_tracker_(nonce_tracker),
      weak_factory_(this) {}
EthPendingTxTracker::~EthPendingTxTracker() = default;

bool EthPendingTxTracker::UpdatePendingTransactions(size_t* num_pending) {
  base::Lock* nonce_lock = nonce_tracker_->GetLock();
  if (!nonce_lock->Try())
    return false;

  auto pending_transactions = tx_state_manager_->GetTransactionsByStatus(
      mojom::TransactionStatus::Submitted, absl::nullopt);
  for (const auto& pending_transaction : pending_transactions) {
    if (IsNonceTaken(static_cast<const EthTxMeta&>(*pending_transaction))) {
      DropTransaction(pending_transaction.get());
      continue;
    }
    std::string id = pending_transaction->id();
    json_rpc_service_->GetTransactionReceipt(
        pending_transaction->tx_hash(),
        base::BindOnce(&EthPendingTxTracker::OnGetTxReceipt,
                       weak_factory_.GetWeakPtr(), std::move(id)));
  }

  nonce_lock->Release();
  *num_pending = pending_transactions.size();
  return true;
}

void EthPendingTxTracker::ResubmitPendingTransactions() {
  // TODO(darkdh): limit the rate of tx publishing
  auto pending_transactions = tx_state_manager_->GetTransactionsByStatus(
      mojom::TransactionStatus::Submitted, absl::nullopt);
  for (const auto& pending_transaction : pending_transactions) {
    auto* pending_eth_transaction =
        static_cast<EthTxMeta*>(pending_transaction.get());
    if (!pending_eth_transaction->tx()->IsSigned()) {
      continue;
    }
    json_rpc_service_->SendRawTransaction(
        pending_eth_transaction->tx()->GetSignedTransaction(),
        base::BindOnce(&EthPendingTxTracker::OnSendRawTransaction,
                       weak_factory_.GetWeakPtr()));
  }
}

void EthPendingTxTracker::Reset() {
  network_nonce_map_.clear();
  dropped_blocks_counter_.clear();
}

void EthPendingTxTracker::OnGetTxReceipt(std::string id,
                                         TransactionReceipt receipt,
                                         mojom::ProviderError error,
                                         const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess)
    return;
  base::Lock* nonce_lock = nonce_tracker_->GetLock();
  if (!nonce_lock->Try())
    return;

  std::unique_ptr<EthTxMeta> meta = tx_state_manager_->GetEthTx(id);
  if (!meta) {
    nonce_lock->Release();
    return;
  }
  if (receipt.status) {
    meta->set_tx_receipt(receipt);
    meta->set_status(mojom::TransactionStatus::Confirmed);
    meta->set_confirmed_time(base::Time::Now());
    tx_state_manager_->AddOrUpdateTx(*meta);
  } else if (ShouldTxDropped(*meta)) {
    DropTransaction(meta.get());
  }

  nonce_lock->Release();
}

void EthPendingTxTracker::OnGetNetworkNonce(std::string address,
                                            uint256_t result,
                                            mojom::ProviderError error,
                                            const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess)
    return;

  network_nonce_map_[address] = result;
}

void EthPendingTxTracker::OnSendRawTransaction(
    const std::string& tx_hash,
    mojom::ProviderError error,
    const std::string& error_message) {}

bool EthPendingTxTracker::IsNonceTaken(const EthTxMeta& meta) {
  auto confirmed_transactions = tx_state_manager_->GetTransactionsByStatus(
      mojom::TransactionStatus::Confirmed, absl::nullopt);
  for (const auto& confirmed_transaction : confirmed_transactions) {
    auto* eth_confirmed_transaction =
        static_cast<EthTxMeta*>(confirmed_transaction.get());
    if (eth_confirmed_transaction->tx()->nonce() == meta.tx()->nonce() &&
        eth_confirmed_transaction->id() != meta.id())
      return true;
  }
  return false;
}

bool EthPendingTxTracker::ShouldTxDropped(const EthTxMeta& meta) {
  const std::string hex_address = meta.from();
  if (network_nonce_map_.find(hex_address) == network_nonce_map_.end()) {
    json_rpc_service_->GetEthTransactionCount(
        hex_address,
        base::BindOnce(&EthPendingTxTracker::OnGetNetworkNonce,
                       weak_factory_.GetWeakPtr(), std::move(hex_address)));
  } else {
    uint256_t network_nonce = network_nonce_map_[hex_address];
    network_nonce_map_.erase(hex_address);
    if (meta.tx()->nonce() < network_nonce)
      return true;
  }

  const std::string tx_hash = meta.tx_hash();
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

void EthPendingTxTracker::DropTransaction(TxMeta* meta) {
  if (!meta)
    return;
  tx_state_manager_->DeleteTx(meta->id());
}

}  // namespace brave_wallet
