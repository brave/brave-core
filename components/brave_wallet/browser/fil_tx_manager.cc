/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_tx_manager.h"

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>

#include "base/notreached.h"
#include "brave/components/brave_wallet/browser/account_resolver_delegate.h"
#include "brave/components/brave_wallet/browser/fil_block_tracker.h"
#include "brave/components/brave_wallet/browser/fil_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/fil_transaction.h"
#include "brave/components/brave_wallet/browser/fil_tx_meta.h"
#include "brave/components/brave_wallet/browser/fil_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

FilTxManager::FilTxManager(TxService& tx_service,
                           JsonRpcService* json_rpc_service,
                           KeyringService& keyring_service,
                           TxStorageDelegate& delegate,
                           AccountResolverDelegate& account_resolver_delegate)
    : TxManager(std::make_unique<FilTxStateManager>(delegate,
                                                    account_resolver_delegate),
                std::make_unique<FilBlockTracker>(json_rpc_service),
                tx_service,
                keyring_service),
      nonce_tracker_(std::make_unique<FilNonceTracker>(&GetFilTxStateManager(),
                                                       json_rpc_service)),
      json_rpc_service_(json_rpc_service) {
  GetFilBlockTracker().AddObserver(this);
}

FilTxManager::~FilTxManager() {
  GetFilBlockTracker().RemoveObserver(this);
}

void FilTxManager::GetEstimatedGas(const std::string& chain_id,
                                   const mojom::AccountIdPtr& from,
                                   const std::optional<url::Origin>& origin,
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
      chain_id, from->address, to, gas_premium, gas_fee_cap, gas_limit, nonce,
      max_fee, value,
      base::BindOnce(&FilTxManager::ContinueAddUnapprovedTransaction,
                     weak_factory_.GetWeakPtr(), chain_id, from.Clone(), origin,
                     std::move(tx), std::move(callback)));
}

void FilTxManager::ContinueAddUnapprovedTransaction(
    const std::string& chain_id,
    const mojom::AccountIdPtr& from,
    const std::optional<url::Origin>& origin,
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
  FilTxMeta meta(from, std::move(tx));
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_origin(
      origin.value_or(url::Origin::Create(GURL("chrome://wallet"))));
  meta.set_created_time(base::Time::Now());
  meta.set_status(mojom::TransactionStatus::Unapproved);
  meta.set_chain_id(chain_id);

  if (!tx_state_manager().AddOrUpdateTx(meta)) {
    std::move(callback).Run(
        false, "", l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  std::move(callback).Run(true, meta.id(), "");
}

void FilTxManager::AddUnapprovedTransaction(
    const std::string& chain_id,
    mojom::TxDataUnionPtr tx_data_union,
    const mojom::AccountIdPtr& from,
    const std::optional<url::Origin>& origin,
    AddUnapprovedTransactionCallback callback) {
  DCHECK(tx_data_union->is_fil_tx_data());
  const bool is_mainnet = chain_id == mojom::kFilecoinMainnet;
  auto tx =
      FilTransaction::FromTxData(is_mainnet, tx_data_union->get_fil_tx_data());
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
    GetEstimatedGas(chain_id, from, origin, std::move(tx_ptr),
                    std::move(callback));
  } else {
    ContinueAddUnapprovedTransaction(
        chain_id, from, origin, std::move(tx_ptr), std::move(callback),
        gas_premium, gas_fee_cap, gas_limit,
        mojom::FilecoinProviderError::kSuccess, "");
  }
}

void FilTxManager::ApproveTransaction(const std::string& tx_meta_id,
                                      ApproveTransactionCallback callback) {
  std::unique_ptr<FilTxMeta> meta = GetFilTxStateManager().GetFilTx(tx_meta_id);
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
    auto from = meta->from()->Clone();
    auto chain_id = meta->chain_id();
    nonce_tracker_->GetNextNonce(
        chain_id, from,
        base::BindOnce(&FilTxManager::OnGetNextNonce,
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
    tx_state_manager().AddOrUpdateTx(*meta);
    LOG(ERROR) << "GetNextNonce failed";
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewFilecoinProviderError(
            mojom::FilecoinProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_GET_NONCE_ERROR));
    return;
  }
  if (keyring_service().IsLockedSync()) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewFilecoinProviderError(
            mojom::FilecoinProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  // DCHECK_LE will eventually be expanded into `CheckOpValueStr` which doesn't
  // have uint256_t overload.
  DCHECK(nonce <= static_cast<uint256_t>(UINT64_MAX));
  meta->tx()->set_nonce(static_cast<uint64_t>(nonce));
  meta->set_status(mojom::TransactionStatus::Approved);
  if (!tx_state_manager().AddOrUpdateTx(*meta)) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewFilecoinProviderError(
            mojom::FilecoinProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto signed_tx = keyring_service().SignTransactionByFilecoinKeyring(
      *meta->from(), meta->tx());
  if (!signed_tx) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewFilecoinProviderError(
            mojom::FilecoinProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_SIGN_TRANSACTION_ERROR));
    return;
  }
  json_rpc_service_->SendFilecoinTransaction(
      meta->chain_id(), signed_tx.value(),
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
  std::unique_ptr<TxMeta> meta = tx_state_manager().GetTx(tx_meta_id);
  if (!meta) {
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

  if (!tx_state_manager().AddOrUpdateTx(*meta)) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewFilecoinProviderError(
            mojom::FilecoinProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  if (success) {
    UpdatePendingTransactions(meta->chain_id());
  }
  std::move(callback).Run(
      error_message.empty(),
      mojom::ProviderErrorUnion::NewFilecoinProviderError(error),
      error_message);
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

void FilTxManager::GetFilTransactionMessageToSign(
    const std::string& tx_meta_id,
    GetFilTransactionMessageToSignCallback callback) {
  std::unique_ptr<FilTxMeta> meta = GetFilTxStateManager().GetFilTx(tx_meta_id);
  if (!meta || !meta->tx()) {
    VLOG(1) << __FUNCTION__ << "No transaction found with id:" << tx_meta_id;
    std::move(callback).Run(std::nullopt);
    return;
  }
  if (!meta->tx()->nonce()) {
    auto from = meta->from().Clone();
    auto chain_id = meta->chain_id();
    nonce_tracker_->GetNextNonce(
        chain_id, from,
        base::BindOnce(&FilTxManager::OnGetNextNonceForHardware,
                       weak_factory_.GetWeakPtr(), std::move(meta),
                       std::move(callback)));
  } else {
    uint64_t nonce = meta->tx()->nonce().value();
    OnGetNextNonceForHardware(std::move(meta), std::move(callback), true,
                              nonce);
  }
}

mojom::CoinType FilTxManager::GetCoinType() const {
  return mojom::CoinType::FIL;
}

void FilTxManager::OnGetNextNonceForHardware(
    std::unique_ptr<FilTxMeta> meta,
    GetFilTransactionMessageToSignCallback callback,
    bool success,
    uint256_t nonce) {
  if (!success) {
    meta->set_status(mojom::TransactionStatus::Error);
    tx_state_manager().AddOrUpdateTx(*meta);
    std::move(callback).Run(std::nullopt);
    return;
  }
  // DCHECK_LE will eventually be expanded into `CheckOpValueStr` which doesn't
  // have uint256_t overload.
  DCHECK(nonce <= static_cast<uint256_t>(UINT64_MAX));
  meta->tx()->set_nonce(static_cast<uint64_t>(nonce));
  meta->set_status(mojom::TransactionStatus::Approved);
  if (!tx_state_manager().AddOrUpdateTx(*meta)) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  auto fil_address = FilAddress::FromAddress(meta->from()->address);
  if (fil_address.IsEmpty()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  std::move(callback).Run(meta->tx()->GetMessageToSignJson(fil_address));
}

void FilTxManager::Reset() {
  TxManager::Reset();
}

std::unique_ptr<FilTxMeta> FilTxManager::GetTxForTesting(
    const std::string& tx_meta_id) {
  return GetFilTxStateManager().GetFilTx(tx_meta_id);
}

FilTxStateManager& FilTxManager::GetFilTxStateManager() {
  return static_cast<FilTxStateManager&>(tx_state_manager());
}

void FilTxManager::UpdatePendingTransactions(
    const std::optional<std::string>& chain_id) {
  auto pending_transactions = tx_state_manager().GetTransactionsByStatus(
      chain_id, mojom::TransactionStatus::Submitted, std::nullopt);
  std::set<std::string> pending_chain_ids;
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
    const auto& pending_chain_id = pending_transaction->chain_id();
    json_rpc_service_->GetFilStateSearchMsgLimited(
        pending_chain_id, cid, limit_epochs,
        base::BindOnce(&FilTxManager::OnGetFilStateSearchMsgLimited,
                       weak_factory_.GetWeakPtr(), pending_transaction->id()));
    pending_chain_ids.emplace(pending_chain_id);
  }
  CheckIfBlockTrackerShouldRun(pending_chain_ids);
}

void FilTxManager::OnGetFilStateSearchMsgLimited(
    const std::string& tx_meta_id,
    int64_t exit_code,
    mojom::FilecoinProviderError error,
    const std::string& error_message) {
  if (error != mojom::FilecoinProviderError::kSuccess) {
    return;
  }
  std::unique_ptr<FilTxMeta> meta = GetFilTxStateManager().GetFilTx(tx_meta_id);
  if (!meta) {
    return;
  }
  mojom::TransactionStatus status = (exit_code == 0)
                                        ? mojom::TransactionStatus::Confirmed
                                        : mojom::TransactionStatus::Error;
  meta->set_status(status);
  if (status == mojom::TransactionStatus::Confirmed) {
    meta->set_confirmed_time(base::Time::Now());
  }
  tx_state_manager().AddOrUpdateTx(*meta);
}

void FilTxManager::OnLatestHeightUpdated(const std::string& chain_id,
                                         uint64_t latest_height) {
  UpdatePendingTransactions(chain_id);
}

FilBlockTracker& FilTxManager::GetFilBlockTracker() {
  return static_cast<FilBlockTracker&>(block_tracker());
}

void FilTxManager::ProcessFilHardwareSignature(
    const std::string& tx_meta_id,
    const mojom::FilecoinSignaturePtr& hw_signature,
    ProcessFilHardwareSignatureCallback callback) {
  std::unique_ptr<FilTxMeta> meta = GetFilTxStateManager().GetFilTx(tx_meta_id);
  if (!meta) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewFilecoinProviderError(
            mojom::FilecoinProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  meta->set_status(mojom::TransactionStatus::Approved);
  if (!tx_state_manager().AddOrUpdateTx(*meta)) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewFilecoinProviderError(
            mojom::FilecoinProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  json_rpc_service_->SendFilecoinTransaction(
      meta->chain_id(), hw_signature->signed_message_json,
      base::BindOnce(&FilTxManager::OnSendFilecoinTransaction,
                     weak_factory_.GetWeakPtr(), meta->id(),
                     std::move(callback)));
}

}  // namespace brave_wallet
