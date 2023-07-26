/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_manager.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/logging.h"
#include "brave/components/brave_wallet/browser/block_tracker.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_service.h"

namespace brave_wallet {

TxManager::TxManager(std::unique_ptr<TxStateManager> tx_state_manager,
                     std::unique_ptr<BlockTracker> block_tracker,
                     TxService* tx_service,
                     JsonRpcService* json_rpc_service,
                     KeyringService* keyring_service,
                     PrefService* prefs)
    : tx_state_manager_(std::move(tx_state_manager)),
      block_tracker_(std::move(block_tracker)),
      tx_service_(tx_service),
      json_rpc_service_(json_rpc_service),
      keyring_service_(keyring_service),
      prefs_(prefs) {
  DCHECK(tx_service_);
  DCHECK(json_rpc_service_);
  DCHECK(keyring_service_);

  tx_state_manager_->AddObserver(this);
  keyring_service_->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());
}

TxManager::~TxManager() {
  tx_state_manager_->RemoveObserver(this);
}

void TxManager::GetTransactionInfo(const std::string& chain_id,
                                   const std::string& tx_meta_id,
                                   GetTransactionInfoCallback callback) {
  std::unique_ptr<TxMeta> meta = tx_state_manager_->GetTx(chain_id, tx_meta_id);
  if (!meta) {
    LOG(ERROR) << "No transaction found";
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(meta->ToTransactionInfo());
}

void TxManager::GetAllTransactionInfo(
    const absl::optional<std::string>& chain_id,
    const absl::optional<std::string>& from,
    GetAllTransactionInfoCallback callback) {
  if (from.has_value() && from->empty()) {
    std::move(callback).Run(std::vector<mojom::TransactionInfoPtr>());
    return;
  }

  std::vector<std::unique_ptr<TxMeta>> metas =
      tx_state_manager_->GetTransactionsByStatus(chain_id, absl::nullopt, from);

  // Convert vector of TxMeta to vector of TransactionInfo
  std::vector<mojom::TransactionInfoPtr> tis(metas.size());
  std::transform(
      metas.begin(), metas.end(), tis.begin(),
      [](const std::unique_ptr<TxMeta>& m) -> mojom::TransactionInfoPtr {
        return m->ToTransactionInfo();
      });
  std::move(callback).Run(std::move(tis));
}

void TxManager::RejectTransaction(const std::string& chain_id,
                                  const std::string& tx_meta_id,
                                  RejectTransactionCallback callback) {
  std::unique_ptr<TxMeta> meta = tx_state_manager_->GetTx(chain_id, tx_meta_id);
  if (!meta) {
    LOG(ERROR) << "No transaction found";
    std::move(callback).Run(false);
    return;
  }
  meta->set_status(mojom::TransactionStatus::Rejected);
  tx_state_manager_->AddOrUpdateTx(*meta);
  std::move(callback).Run(true);
}

void TxManager::CheckIfBlockTrackerShouldRun(
    const std::set<std::string>& new_pending_chain_ids) {
  bool locked = keyring_service_->IsLockedSync();

  if (locked) {
    block_tracker_->Stop();
  } else if (new_pending_chain_ids != pending_chain_ids_) {
    // Stop tracker that is no longer needed.
    for (const auto& pending_chain_id : pending_chain_ids_) {
      if (!base::Contains(new_pending_chain_ids, pending_chain_id)) {
        block_tracker_->Stop(pending_chain_id);
      }
    }
    for (const auto& chain_id : new_pending_chain_ids) {
      bool running = block_tracker_->IsRunning(chain_id);
      if (!running) {
        block_tracker_->Start(chain_id,
                              base::Seconds(kBlockTrackerDefaultTimeInSeconds));
      }
    }
    pending_chain_ids_ = new_pending_chain_ids;
  }
}

void TxManager::OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) {
  tx_service_->OnTransactionStatusChanged(tx_info->Clone());
}

void TxManager::OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) {
  tx_service_->OnNewUnapprovedTx(tx_info->Clone());
}

void TxManager::Locked() {
  block_tracker_->Stop();
}

void TxManager::Unlocked() {
  UpdatePendingTransactions(absl::nullopt);
}

void TxManager::KeyringReset() {
  Reset();
}

void TxManager::Reset() {
  block_tracker_->Stop();
  pending_chain_ids_.clear();
}

}  // namespace brave_wallet
