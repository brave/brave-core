/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_manager.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/logging.h"
#include "brave/components/brave_wallet/browser/block_tracker.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
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

  // TODO(darkdh): Check if this is needed. (Unlock should be sufficient)
  // CheckIfBlockTrackerShouldRun(absl::nullopt);
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

void TxManager::CheckIfBlockTrackerShouldRun(const std::string& chain_id) {
  const auto& keyring_id = CoinTypeToKeyringId(GetCoinType(), chain_id);
  CHECK(keyring_id.has_value());
  bool keyring_created = keyring_service_->IsKeyringCreated(*keyring_id);
  bool locked = keyring_service_->IsLockedSync();
  bool running = block_tracker_->IsRunning(chain_id);
  if (keyring_created && !locked && !running) {
    block_tracker_->Start(chain_id,
                          base::Seconds(kBlockTrackerDefaultTimeInSeconds));
  } else if ((locked || base::Contains(known_no_pending_tx_, chain_id)) &&
             running) {
    block_tracker_->Stop(chain_id);
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
  const std::string& chain_id =
      json_rpc_service_->GetChainId(GetCoinType(), absl::nullopt);
  CheckIfBlockTrackerShouldRun(chain_id);
  UpdatePendingTransactions(chain_id);
}

void TxManager::KeyringCreated(const std::string& keyring_id) {
  /* TODO(darkdh): Check if this is needed (There should be no pending txs)
  const auto coin = GetCoinForKeyring(keyring_id);
  UpdatePendingTransactions(json_rpc_service_->GetChainId(coin, absl::nullopt));
  */
}

void TxManager::KeyringRestored(const std::string& keyring_id) {
  /* TODO(darkdh): Check if this is needed (There should be no pending txs)
  const auto coin = GetCoinForKeyring(keyring_id);
  UpdatePendingTransactions(json_rpc_service_->GetChainId(coin, absl::nullopt));
  */
}

void TxManager::KeyringReset() {
  Reset();
}

void TxManager::Reset() {
  block_tracker_->Stop();
  known_no_pending_tx_.clear();
}

}  // namespace brave_wallet
