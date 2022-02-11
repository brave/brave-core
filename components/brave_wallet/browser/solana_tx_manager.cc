/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_tx_manager.h"

#include "base/notreached.h"

namespace brave_wallet {

SolanaTxManager::SolanaTxManager(TxService* tx_service,
                                 JsonRpcService* json_rpc_service,
                                 KeyringService* keyring_service,
                                 PrefService* prefs)
    : TxManager(tx_service, json_rpc_service, keyring_service, prefs) {}

void SolanaTxManager::AddUnapprovedTransaction(
    mojom::TxDataUnionPtr tx_data_union,
    const std::string& from,
    AddUnapprovedTransactionCallback) {
  NOTIMPLEMENTED();
}

void SolanaTxManager::ApproveTransaction(const std::string& tx_meta_id,
                                         ApproveTransactionCallback) {
  NOTIMPLEMENTED();
}

void SolanaTxManager::RejectTransaction(const std::string& tx_meta_id,
                                        RejectTransactionCallback) {
  NOTIMPLEMENTED();
}

void SolanaTxManager::GetAllTransactionInfo(const std::string& from,
                                            GetAllTransactionInfoCallback) {
  NOTIMPLEMENTED();
}

void SolanaTxManager::SpeedupOrCancelTransaction(
    const std::string& tx_meta_id,
    bool cancel,
    SpeedupOrCancelTransactionCallback callback) {
  NOTIMPLEMENTED();
}

void SolanaTxManager::RetryTransaction(const std::string& tx_meta_id,
                                       RetryTransactionCallback callback) {
  NOTIMPLEMENTED();
}

void SolanaTxManager::GetTransactionMessageToSign(
    const std::string& tx_meta_id,
    GetTransactionMessageToSignCallback callback) {
  NOTIMPLEMENTED();
}

void SolanaTxManager::Reset() {
  // TODO(jocelyn): reset members as necessary.
}

}  // namespace brave_wallet
