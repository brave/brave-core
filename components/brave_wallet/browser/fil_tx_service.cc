/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_tx_service.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/eth_data_parser.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

FilTxService::FilTxService(
    JsonRpcService* json_rpc_service,
    KeyringService* keyring_service,
    AssetRatioService* asset_ratio_service,
    std::unique_ptr<EthTxStateManager> tx_state_manager,
    std::unique_ptr<EthNonceTracker> nonce_tracker,
    std::unique_ptr<EthPendingTxTracker> pending_tx_tracker,
    PrefService* prefs)
    : json_rpc_service_(json_rpc_service),
      keyring_service_(keyring_service),
      asset_ratio_service_(asset_ratio_service),
      prefs_(prefs),
      tx_state_manager_(std::move(tx_state_manager)),
      nonce_tracker_(std::move(nonce_tracker)),
      pending_tx_tracker_(std::move(pending_tx_tracker)),
      weak_factory_(this) {
  DCHECK(json_rpc_service_);
  DCHECK(keyring_service_);
  tx_state_manager_->AddObserver(this);
  keyring_service_->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());
}

FilTxService::~FilTxService() {
  tx_state_manager_->RemoveObserver(this);
}

mojo::PendingRemote<mojom::FilTxService> FilTxService::MakeRemote() {
  mojo::PendingRemote<mojom::FilTxService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void FilTxService::Bind(mojo::PendingReceiver<mojom::FilTxService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void FilTxService::AddUnapprovedTransaction(
    mojom::TxDataPtr tx_data,
    const std::string& from,
    AddUnapprovedTransactionCallback callback) {
  // NOT IMPLEMENTED
  std::move(callback).Run(false, "", "");
}

void FilTxService::OnGetGasPrice(const std::string& from,
                                 const std::string& to,
                                 const std::string& value,
                                 const std::string& data,
                                 const std::string& gas_limit,
                                 std::unique_ptr<EthTransaction> tx,
                                 AddUnapprovedTransactionCallback callback,
                                 const std::string& result,
                                 mojom::ProviderError error,
                                 const std::string& error_message) {
  // NOT IMPLEMENTED
}

void FilTxService::ContinueAddUnapprovedTransaction(
    const std::string& from,
    std::unique_ptr<EthTransaction> tx,
    AddUnapprovedTransactionCallback callback,
    const std::string& result,
    mojom::ProviderError error,
    const std::string& error_message) {
  // NOT IMPLEMENTED
}

void FilTxService::GetNonceForHardwareTransaction(
    const std::string& tx_meta_id,
    GetNonceForHardwareTransactionCallback callback) {
  // NOT IMPLEMENTED
  std::move(callback).Run(absl::nullopt);
}

void FilTxService::GetTransactionMessageToSign(
    const std::string& tx_meta_id,
    GetTransactionMessageToSignCallback callback) {
  // NOT IMPLEMENTED
  std::move(callback).Run(absl::nullopt);
}

void FilTxService::OnGetNextNonceForHardware(
    std::unique_ptr<EthTxStateManager::TxMeta> meta,
    GetNonceForHardwareTransactionCallback callback,
    bool success,
    uint256_t nonce) {
  // NOT IMPLEMENTED
}

void FilTxService::ProcessHardwareSignature(
    const std::string& tx_meta_id,
    const std::string& v,
    const std::string& r,
    const std::string& s,
    ProcessHardwareSignatureCallback callback) {
  // NOT IMPLEMENTED
  std::move(callback).Run(false);
}

void FilTxService::ApproveTransaction(const std::string& tx_meta_id,
                                      ApproveTransactionCallback callback) {
  // NOT IMPLEMENTED
  std::move(callback).Run(false);
}

void FilTxService::RejectTransaction(const std::string& tx_meta_id,
                                     RejectTransactionCallback callback) {
  // NOT IMPLEMENTED
  std::move(callback).Run(false);
}

void FilTxService::OnGetNextNonce(
    std::unique_ptr<EthTxStateManager::TxMeta> meta,
    uint256_t chain_id,
    ApproveTransactionCallback callback,
    bool success,
    uint256_t nonce) {
  // NOT IMPLEMENTED
}

void FilTxService::PublishTransaction(const std::string& tx_meta_id,
                                      const std::string& signed_transaction,
                                      ApproveTransactionCallback callback) {
  // NOT IMPLEMENTED
}

void FilTxService::OnPublishTransaction(std::string tx_meta_id,
                                        ApproveTransactionCallback callback,
                                        const std::string& tx_hash,
                                        mojom::ProviderError error,
                                        const std::string& error_message) {
  // NOT IMPLEMENTED
  std::move(callback).Run(false);
}

void FilTxService::AddObserver(
    ::mojo::PendingRemote<mojom::EthTxServiceObserver> observer) {
  observers_.Add(std::move(observer));
}

void FilTxService::OnTransactionStatusChanged(
    mojom::TransactionInfoPtr tx_info) {
  for (const auto& observer : observers_)
    observer->OnTransactionStatusChanged(tx_info->Clone());
}

void FilTxService::OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) {
  for (const auto& observer : observers_)
    observer->OnNewUnapprovedTx(tx_info->Clone());
}

void FilTxService::NotifyUnapprovedTxUpdated(EthTxStateManager::TxMeta* meta) {
  for (const auto& observer : observers_)
    observer->OnUnapprovedTxUpdated(
        EthTxStateManager::TxMetaToTransactionInfo(*meta));
}

void FilTxService::GetAllTransactionInfo(
    const std::string& from,
    GetAllTransactionInfoCallback callback) {
  // NOT IMPLEMENTED
  std::move(callback).Run(std::vector<mojom::TransactionInfoPtr>());
}

std::unique_ptr<EthTxStateManager::TxMeta> FilTxService::GetTxForTesting(
    const std::string& tx_meta_id) {
  return tx_state_manager_->GetTx(tx_meta_id);
}

void FilTxService::UpdatePendingTransactions() {
  // NOT IMPLEMENTED
}

void FilTxService::Locked() {
  // NOT IMPLEMENTED
}

void FilTxService::Unlocked() {
  UpdatePendingTransactions();
}

void FilTxService::KeyringCreated(const std::string& keyring_id) {
  UpdatePendingTransactions();
}

void FilTxService::KeyringRestored(const std::string& keyring_id) {
  UpdatePendingTransactions();
}

void FilTxService::KeyringReset() {
  UpdatePendingTransactions();
}

void FilTxService::RetryTransaction(const std::string& tx_meta_id,
                                    RetryTransactionCallback callback) {
  // NOT IMPLEMENTED
}

void FilTxService::Reset() {
  ClearTxServiceProfilePrefs(prefs_);
  pending_tx_tracker_->Reset();
  known_no_pending_tx = false;
}

}  // namespace brave_wallet
