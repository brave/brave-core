/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_tx_manager.h"

#include <memory>

#include "base/notreached.h"
#include "brave/components/brave_wallet/browser/fil_block_tracker.h"
#include "brave/components/brave_wallet/browser/fil_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/fil_tx_state_manager.h"

namespace brave_wallet {

FilTxManager::FilTxManager(TxService* tx_service,
                           JsonRpcService* json_rpc_service,
                           KeyringService* keyring_service,
                           PrefService* prefs)
    : TxManager(std::make_unique<FilTxStateManager>(prefs, json_rpc_service),
                std::make_unique<FilBlockTracker>(json_rpc_service),
                tx_service,
                json_rpc_service,
                keyring_service,
                prefs),
      nonce_tracker_(std::make_unique<FilNonceTracker>(GetFilTxStateManager(),
                                                       json_rpc_service)) {}

FilTxManager::~FilTxManager() = default;

void FilTxManager::AddUnapprovedTransaction(mojom::TxDataUnionPtr tx_data_union,
                                            const std::string& from,
                                            AddUnapprovedTransactionCallback) {
  NOTIMPLEMENTED();
}

void FilTxManager::ApproveTransaction(const std::string& tx_meta_id,
                                      ApproveTransactionCallback) {
  NOTIMPLEMENTED();
}

void FilTxManager::RejectTransaction(const std::string& tx_meta_id,
                                     RejectTransactionCallback) {
  NOTIMPLEMENTED();
}

void FilTxManager::GetAllTransactionInfo(const std::string& from,
                                         GetAllTransactionInfoCallback) {
  NOTIMPLEMENTED();
}

void FilTxManager::SpeedupOrCancelTransaction(
    const std::string& tx_meta_id,
    bool cancel,
    SpeedupOrCancelTransactionCallback callback) {
  NOTIMPLEMENTED();
}

void FilTxManager::RetryTransaction(const std::string& tx_meta_id,
                                    RetryTransactionCallback callback) {
  NOTIMPLEMENTED();
}

void FilTxManager::GetTransactionMessageToSign(
    const std::string& tx_meta_id,
    GetTransactionMessageToSignCallback callback) {
  NOTIMPLEMENTED();
}

void FilTxManager::Reset() {
  TxManager::Reset();
  // TODO(spylogsster): reset members as necessary.
}

void FilTxManager::UpdatePendingTransactions() {
  NOTIMPLEMENTED();
}

FilTxStateManager* FilTxManager::GetFilTxStateManager() {
  return static_cast<FilTxStateManager*>(tx_state_manager_.get());
}

}  // namespace brave_wallet
