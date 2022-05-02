/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_tx_manager.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/notreached.h"
#include "brave/components/brave_wallet/browser/fil_block_tracker.h"
#include "brave/components/brave_wallet/browser/fil_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/fil_transaction.h"
#include "brave/components/brave_wallet/browser/fil_tx_meta.h"
#include "brave/components/brave_wallet/browser/fil_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

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
                                                       json_rpc_service)) {
  GetFilBlockTracker()->AddObserver(this);
}

FilTxManager::~FilTxManager() {
  GetFilBlockTracker()->RemoveObserver(this);
}

void FilTxManager::GetEstimatedGas(const std::string& from,
                                   const absl::optional<url::Origin>& origin,
                                   std::unique_ptr<FilTransaction> tx,
                                   AddUnapprovedTransactionCallback callback) {
  const std::string gas_premium = tx->gas_premium();
  const std::string gas_fee_cap = tx->gas_fee_cap();
  auto gas_limit = tx->gas_limit();
  uint64_t nonce = tx->nonce() ? tx->nonce().value() : 0;
  const std::string value = tx->value();
  const std::string max_fee = tx->max_fee();
  auto to = tx->to().EncodeAsString();
  json_rpc_service_->GetFilEstimateGas(
      from, to, gas_premium, gas_fee_cap, gas_limit, nonce, max_fee, value,
      base::BindOnce(&FilTxManager::ContinueAddUnapprovedTransaction,
                     weak_factory_.GetWeakPtr(), from, origin, std::move(tx),
                     std::move(callback)));
}

void FilTxManager::ContinueAddUnapprovedTransaction(
    const std::string& from,
    const absl::optional<url::Origin>& origin,
    std::unique_ptr<FilTransaction> tx,
    AddUnapprovedTransactionCallback callback,
    const std::string& gas_premium,
    const std::string& gas_fee_cap,
    int64_t gas_limit,
    mojom::FilecoinProviderError error,
    const std::string& error_message) {
  if (error != mojom::FilecoinProviderError::kSuccess) {
    std::move(callback).Run(false, "", error_message);
    return;
  }

  tx->set_gas_premium(gas_premium);
  tx->set_fee_cap(gas_fee_cap);
  tx->set_gas_limit(gas_limit);
  FilTxMeta meta(std::move(tx));
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_from(FilAddress::FromAddress(from).EncodeAsString());
  meta.set_origin(
      origin.value_or(url::Origin::Create(GURL("chrome://wallet"))));
  meta.set_created_time(base::Time::Now());
  meta.set_status(mojom::TransactionStatus::Unapproved);
  tx_state_manager_->AddOrUpdateTx(meta);
  std::move(callback).Run(true, meta.id(), "");
}

void FilTxManager::AddUnapprovedTransaction(
    mojom::TxDataUnionPtr tx_data_union,
    const std::string& from,
    const absl::optional<url::Origin>& origin,
    AddUnapprovedTransactionCallback callback) {
  DCHECK(tx_data_union->is_fil_tx_data());
  if (!FilAddress::IsValidAddress(from)) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_FIL_SEND_TRANSACTION_FROM_INVALID));
    return;
  }
  auto tx =
      FilTransaction::FromTxData(std::move(tx_data_union->get_fil_tx_data()));
  if (!tx) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_SEND_TRANSACTION_CONVERT_TX_DATA));
    return;
  }
  if (!FilAddress::IsValidAddress(tx->to().EncodeAsString())) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_FIL_SEND_TRANSACTION_TO));
    return;
  }

  auto tx_ptr = std::make_unique<FilTransaction>(*tx);
  const std::string gas_fee_cap = tx->gas_fee_cap();
  const std::string gas_premium = tx->gas_premium();
  auto gas_limit = tx->gas_limit();
  if (!gas_limit || gas_fee_cap.empty() || gas_premium.empty()) {
    GetEstimatedGas(from, origin, std::move(tx_ptr), std::move(callback));
  } else {
    ContinueAddUnapprovedTransaction(
        from, origin, std::move(tx_ptr), std::move(callback), gas_premium,
        gas_fee_cap, gas_limit, mojom::FilecoinProviderError::kSuccess, "");
  }
}

void FilTxManager::ApproveTransaction(const std::string& tx_meta_id,
                                      ApproveTransactionCallback callback) {
  std::unique_ptr<FilTxMeta> meta =
      GetFilTxStateManager()->GetFilTx(tx_meta_id);
  if (!meta) {
    DCHECK(false) << "Transaction should be found";
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewFilecoinProviderError(
            mojom::FilecoinProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }
  if (!meta->tx()->nonce()) {
    auto from = meta->from();
    nonce_tracker_->GetNextNonce(
        from, base::BindOnce(&FilTxManager::OnGetNextNonce,
                             weak_factory_.GetWeakPtr(), std::move(meta),
                             std::move(callback)));
  } else {
    uint64_t nonce = meta->tx()->nonce().value();
    OnGetNextNonce(std::move(meta), std::move(callback), true, nonce);
  }
}

void FilTxManager::OnGetNextNonce(std::unique_ptr<FilTxMeta> meta,
                                  ApproveTransactionCallback callback,
                                  bool success,
                                  uint256_t nonce) {
  if (!success) {
    meta->set_status(mojom::TransactionStatus::Error);
    tx_state_manager_->AddOrUpdateTx(*meta);
    LOG(ERROR) << "GetNextNonce failed";
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewFilecoinProviderError(
            mojom::FilecoinProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_GET_NONCE_ERROR));
    return;
  }
  DCHECK_LE(nonce, static_cast<uint256_t>(UINT64_MAX));
  meta->tx()->set_nonce(static_cast<uint64_t>(nonce));
  DCHECK(!keyring_service_->IsLocked());
  meta->set_status(mojom::TransactionStatus::Approved);
  tx_state_manager_->AddOrUpdateTx(*meta);

  auto signed_tx =
      keyring_service_->SignTransactionByFilecoinKeyring(meta->tx());
  if (!signed_tx) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewFilecoinProviderError(
            mojom::FilecoinProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_SIGN_TRANSACTION_ERROR));
    return;
  }
  json_rpc_service_->SendFilecoinTransaction(
      signed_tx.value(),
      base::BindOnce(&FilTxManager::OnSendFilecoinTransaction,
                     weak_factory_.GetWeakPtr(), meta->id(),
                     std::move(callback)));
}

void FilTxManager::OnSendFilecoinTransaction(
    const std::string& tx_meta_id,
    ApproveTransactionCallback callback,
    const std::string& tx_cid,
    mojom::FilecoinProviderError error,
    const std::string& error_message) {
  std::unique_ptr<TxMeta> meta = tx_state_manager_->GetTx(tx_meta_id);
  if (!meta) {
    NOTREACHED() << "Transaction should be found";
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewFilecoinProviderError(
            mojom::FilecoinProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  bool success = error == mojom::FilecoinProviderError::kSuccess;

  if (success) {
    meta->set_status(mojom::TransactionStatus::Submitted);
    meta->set_submitted_time(base::Time::Now());
    meta->set_tx_hash(tx_cid);
  } else {
    meta->set_status(mojom::TransactionStatus::Error);
  }

  tx_state_manager_->AddOrUpdateTx(*meta);

  if (success)
    UpdatePendingTransactions();
  std::move(callback).Run(
      error_message.empty(),
      mojom::ProviderErrorUnion::NewFilecoinProviderError(error),
      error_message);
}

void FilTxManager::GetAllTransactionInfo(
    const std::string& from,
    GetAllTransactionInfoCallback callback) {
  auto from_address = FilAddress::FromAddress(from);
  if (from_address.IsEmpty()) {
    std::move(callback).Run(std::vector<mojom::TransactionInfoPtr>());
    return;
  }
  TxManager::GetAllTransactionInfo(from_address.EncodeAsString(),
                                   std::move(callback));
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
  std::unique_ptr<FilTxMeta> meta =
      GetFilTxStateManager()->GetFilTx(tx_meta_id);
  if (!meta || !meta->tx()) {
    VLOG(1) << __FUNCTION__ << "No transaction found with id:" << tx_meta_id;
    std::move(callback).Run(absl::nullopt);
    return;
  }
  std::move(callback).Run(meta->tx()->GetMessageToSign());
}

void FilTxManager::Reset() {
  TxManager::Reset();
}

std::unique_ptr<FilTxMeta> FilTxManager::GetTxForTesting(
    const std::string& tx_meta_id) {
  return GetFilTxStateManager()->GetFilTx(tx_meta_id);
}

FilTxStateManager* FilTxManager::GetFilTxStateManager() {
  return static_cast<FilTxStateManager*>(tx_state_manager_.get());
}

void FilTxManager::UpdatePendingTransactions() {
  auto pending_transactions = tx_state_manager_->GetTransactionsByStatus(
      mojom::TransactionStatus::Submitted, absl::nullopt);
  for (const auto& pending_transaction : pending_transactions) {
    auto cid = pending_transaction->tx_hash();
    uint64_t seconds =
        (base::Time::Now() - pending_transaction->submitted_time()).InSeconds();
    // StateSearchMsgLimited looks back up to limit epochs in the chain for a
    // message. Returns its receipt and the tipset where it was executed.
    // We assume that 1 block mined per second and taking a
    // difference between current time and transaction submission to calculate
    // the limit. In case of zero we are looking at last message.
    uint64_t limit_epochs = seconds == 0 ? 1 : seconds;
    json_rpc_service_->GetFilStateSearchMsgLimited(
        cid, limit_epochs,
        base::BindOnce(&FilTxManager::OnGetFilStateSearchMsgLimited,
                       weak_factory_.GetWeakPtr(), pending_transaction->id()));
  }
  known_no_pending_tx_ = pending_transactions.empty();
  CheckIfBlockTrackerShouldRun();
}

void FilTxManager::OnGetFilStateSearchMsgLimited(
    const std::string& tx_meta_id,
    int64_t exit_code,
    mojom::FilecoinProviderError error,
    const std::string& error_message) {
  if (error != mojom::FilecoinProviderError::kSuccess)
    return;
  std::unique_ptr<FilTxMeta> meta =
      GetFilTxStateManager()->GetFilTx(tx_meta_id);
  if (!meta)
    return;
  mojom::TransactionStatus status = (exit_code == 0)
                                        ? mojom::TransactionStatus::Confirmed
                                        : mojom::TransactionStatus::Error;
  meta->set_status(status);
  if (status == mojom::TransactionStatus::Confirmed) {
    meta->set_confirmed_time(base::Time::Now());
  }
  tx_state_manager_->AddOrUpdateTx(*meta);
}

void FilTxManager::OnLatestHeightUpdated(uint64_t latest_height) {
  UpdatePendingTransactions();
}

FilBlockTracker* FilTxManager::GetFilBlockTracker() {
  return static_cast<FilBlockTracker*>(block_tracker_.get());
}

void FilTxManager::ProcessFilHardwareSignature(
    const std::string& tx_meta_id,
    const std::string& signed_tx,
    ProcessFilHardwareSignatureCallback callback) {
  std::unique_ptr<FilTxMeta> meta =
      GetFilTxStateManager()->GetFilTx(tx_meta_id);
  if (!meta) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewFilecoinProviderError(
            mojom::FilecoinProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  meta->set_status(mojom::TransactionStatus::Confirmed);
  meta->set_confirmed_time(base::Time::Now());
  tx_state_manager_->AddOrUpdateTx(*meta);

  json_rpc_service_->SendFilecoinTransaction(
      signed_tx, base::BindOnce(&FilTxManager::OnSendFilecoinTransaction,
                                weak_factory_.GetWeakPtr(), meta->id(),
                                std::move(callback)));
}

}  // namespace brave_wallet
