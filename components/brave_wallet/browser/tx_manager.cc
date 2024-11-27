/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_manager.h"

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/logging.h"
#include "brave/components/brave_wallet/browser/block_tracker.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_service.h"

namespace brave_wallet {

TxManager::TxManager(std::unique_ptr<TxStateManager> tx_state_manager,
                     std::unique_ptr<BlockTracker> block_tracker,
                     TxService& tx_service,
                     KeyringService& keyring_service)
    : tx_state_manager_(std::move(tx_state_manager)),
      block_tracker_(std::move(block_tracker)),
      tx_service_(tx_service),
      keyring_service_(keyring_service) {
  tx_state_manager_->AddObserver(this);
  keyring_service_->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());
}

TxManager::~TxManager() {
  tx_state_manager_->RemoveObserver(this);
}

mojom::TransactionInfoPtr TxManager::GetTransactionInfo(
    const std::string& tx_meta_id) {
  std::unique_ptr<TxMeta> meta = tx_state_manager_->GetTx(tx_meta_id);
  if (!meta) {
    LOG(ERROR) << "No transaction found";

    return nullptr;
  }

  return meta->ToTransactionInfo();
}

std::vector<mojom::TransactionInfoPtr> TxManager::GetAllTransactionInfo(
    const std::optional<std::string>& chain_id,
    const std::optional<mojom::AccountIdPtr>& from) {
  auto metas =
      tx_state_manager_->GetTransactionsByStatus(chain_id, std::nullopt, from);

  // Convert vector of TxMeta to vector of TransactionInfo
  std::vector<mojom::TransactionInfoPtr> tis(metas.size());
  std::transform(
      metas.begin(), metas.end(), tis.begin(),
      [](const std::unique_ptr<TxMeta>& m) -> mojom::TransactionInfoPtr {
        return m->ToTransactionInfo();
      });
  return tis;
}

void TxManager::RejectTransaction(const std::string& tx_meta_id,
                                  RejectTransactionCallback callback) {
  std::unique_ptr<TxMeta> meta = tx_state_manager_->GetTx(tx_meta_id);
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
      if (!new_pending_chain_ids.contains(pending_chain_id)) {
        block_tracker_->Stop(pending_chain_id);
      }
    }
    for (const auto& chain_id : new_pending_chain_ids) {
      bool running = block_tracker_->IsRunning(chain_id);
      auto interval = GetCoinType() == mojom::CoinType::SOL
                          ? base::Seconds(kSolanaBlockTrackerTimeInSeconds)
                          : base::Seconds(kBlockTrackerDefaultTimeInSeconds);
      if (!running) {
        block_tracker_->Start(chain_id, interval);
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
  UpdatePendingTransactions(std::nullopt);
}

void TxManager::WalletReset() {
  Reset();
}

void TxManager::Reset() {
  block_tracker_->Stop();
  pending_chain_ids_.clear();
}

}  // namespace brave_wallet
