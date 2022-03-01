/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_manager.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/logging.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_state_manager.h"

namespace brave_wallet {

TxManager::TxManager(std::unique_ptr<TxStateManager> tx_state_manager,
                     TxService* tx_service,
                     JsonRpcService* json_rpc_service,
                     KeyringService* keyring_service,
                     PrefService* prefs)
    : tx_state_manager_(std::move(tx_state_manager)),
      tx_service_(tx_service),
      json_rpc_service_(json_rpc_service),
      keyring_service_(keyring_service),
      prefs_(prefs) {
  DCHECK(tx_service_);
  DCHECK(json_rpc_service_);
  DCHECK(keyring_service_);
}

TxManager::~TxManager() = default;

void TxManager::GetAllTransactionInfo(const std::string& from,
                                      GetAllTransactionInfoCallback callback) {
  if (from.empty()) {
    std::move(callback).Run(std::vector<mojom::TransactionInfoPtr>());
    return;
  }

  std::vector<std::unique_ptr<TxMeta>> metas =
      tx_state_manager_->GetTransactionsByStatus(absl::nullopt, from);

  // Convert vector of TxMeta to vector of TransactionInfo
  std::vector<mojom::TransactionInfoPtr> tis(metas.size());
  std::transform(
      metas.begin(), metas.end(), tis.begin(),
      [](const std::unique_ptr<TxMeta>& m) -> mojom::TransactionInfoPtr {
        return m->ToTransactionInfo();
      });
  std::move(callback).Run(std::move(tis));
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

}  // namespace brave_wallet
