/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_tx_manager.h"

#include <memory>
#include <utility>

#include "base/notreached.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/solana_block_tracker.h"
#include "brave/components/brave_wallet/browser/solana_tx_meta.h"
#include "brave/components/brave_wallet/browser/solana_tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

SolanaTxManager::SolanaTxManager(TxService* tx_service,
                                 JsonRpcService* json_rpc_service,
                                 KeyringService* keyring_service,
                                 PrefService* prefs)
    : TxManager(std::make_unique<SolanaTxStateManager>(prefs, json_rpc_service),
                std::make_unique<SolanaBlockTracker>(json_rpc_service),
                tx_service,
                json_rpc_service,
                keyring_service,
                prefs),
      weak_ptr_factory_(this) {
  GetSolanaBlockTracker()->AddObserver(this);
}

SolanaTxManager::~SolanaTxManager() {
  GetSolanaBlockTracker()->RemoveObserver(this);
}

void SolanaTxManager::AddUnapprovedTransaction(
    mojom::TxDataUnionPtr tx_data_union,
    const std::string& from,
    AddUnapprovedTransactionCallback callback) {
  DCHECK(tx_data_union->is_solana_tx_data());

  if (from.empty()) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_SEND_TRANSACTION_FROM_EMPTY));
    return;
  }

  auto tx = SolanaTransaction::FromSolanaTxData(
      std::move(tx_data_union->get_solana_tx_data()));
  if (!tx) {
    std::move(callback).Run(
        false, "",
        l10n_util::GetStringUTF8(IDS_WALLET_SEND_TRANSACTION_CONVERT_TX_DATA));
    return;
  }

  SolanaTxMeta meta(std::move(tx));
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_from(from);
  meta.set_created_time(base::Time::Now());
  meta.set_status(mojom::TransactionStatus::Unapproved);
  tx_state_manager_->AddOrUpdateTx(meta);
  std::move(callback).Run(true, meta.id(), "");
}

void SolanaTxManager::ApproveTransaction(const std::string& tx_meta_id,
                                         ApproveTransactionCallback callback) {
  std::unique_ptr<SolanaTxMeta> meta =
      GetSolanaTxStateManager()->GetSolanaTx(tx_meta_id);
  if (!meta) {
    DCHECK(false) << "Transaction should be found";
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewSolanaProviderError(
            mojom::SolanaProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  // Try to use available latest blockhash from block tracker first.
  std::string hash = GetSolanaBlockTracker()->latest_blockhash();
  if (!hash.empty()) {
    OnGetLatestBlockhash(std::move(meta), std::move(callback), hash,
                         mojom::SolanaProviderError::kSuccess, "");
    return;
  }

  json_rpc_service_->GetSolanaLatestBlockhash(base::BindOnce(
      &SolanaTxManager::OnGetLatestBlockhash, weak_ptr_factory_.GetWeakPtr(),
      std::move(meta), std::move(callback)));
}

void SolanaTxManager::OnGetLatestBlockhash(std::unique_ptr<SolanaTxMeta> meta,
                                           ApproveTransactionCallback callback,
                                           const std::string& latest_blockhash,
                                           mojom::SolanaProviderError error,
                                           const std::string& error_message) {
  if (error != mojom::SolanaProviderError::kSuccess) {
    std::move(callback).Run(
        false, mojom::ProviderErrorUnion::NewSolanaProviderError(error),
        error_message);
    return;
  }

  meta->set_status(mojom::TransactionStatus::Approved);
  tx_state_manager_->AddOrUpdateTx(*meta);

  json_rpc_service_->SendSolanaTransaction(
      meta->tx()->GetSignedTransaction(keyring_service_, latest_blockhash),
      base::BindOnce(&SolanaTxManager::OnSendSolanaTransaction,
                     weak_ptr_factory_.GetWeakPtr(), meta->id(),
                     std::move(callback)));
}

void SolanaTxManager::OnSendSolanaTransaction(
    const std::string& tx_meta_id,
    ApproveTransactionCallback callback,
    const std::string& tx_hash,
    mojom::SolanaProviderError error,
    const std::string& error_message) {
  std::unique_ptr<TxMeta> meta = tx_state_manager_->GetTx(tx_meta_id);
  if (!meta) {
    DCHECK(false) << "Transaction should be found";
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewSolanaProviderError(
            mojom::SolanaProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  bool success = error == mojom::SolanaProviderError::kSuccess;

  if (success) {
    meta->set_status(mojom::TransactionStatus::Submitted);
    meta->set_submitted_time(base::Time::Now());
    meta->set_tx_hash(tx_hash);
  } else {
    meta->set_status(mojom::TransactionStatus::Error);
  }

  tx_state_manager_->AddOrUpdateTx(*meta);

  if (success)
    UpdatePendingTransactions();

  std::move(callback).Run(
      error_message.empty(),
      mojom::ProviderErrorUnion::NewSolanaProviderError(error), error_message);
}

void SolanaTxManager::UpdatePendingTransactions() {
  auto pending_transactions = tx_state_manager_->GetTransactionsByStatus(
      mojom::TransactionStatus::Submitted, absl::nullopt);
  std::vector<std::string> tx_meta_ids;
  std::vector<std::string> tx_signatures;
  for (const auto& pending_transaction : pending_transactions) {
    tx_meta_ids.push_back(pending_transaction->id());
    tx_signatures.push_back(pending_transaction->tx_hash());
  }
  json_rpc_service_->GetSolanaSignatureStatuses(
      tx_signatures,
      base::BindOnce(&SolanaTxManager::OnGetSignatureStatuses,
                     weak_ptr_factory_.GetWeakPtr(), tx_meta_ids));
  known_no_pending_tx_ = pending_transactions.empty();
  CheckIfBlockTrackerShouldRun();
}

void SolanaTxManager::OnGetSignatureStatuses(
    const std::vector<std::string>& tx_meta_ids,
    const std::vector<absl::optional<SolanaSignatureStatus>>&
        signature_statuses,
    mojom::SolanaProviderError error,
    const std::string& error_message) {
  if (error != mojom::SolanaProviderError::kSuccess)
    return;

  if (tx_meta_ids.size() != signature_statuses.size())
    return;

  for (size_t i = 0; i < tx_meta_ids.size(); i++) {
    std::unique_ptr<SolanaTxMeta> meta =
        GetSolanaTxStateManager()->GetSolanaTx(tx_meta_ids[i]);
    if (!meta || !signature_statuses[i])
      continue;

    if (!signature_statuses[i]->err.empty()) {
      meta->set_signature_status(*signature_statuses[i]);
      meta->set_status(mojom::TransactionStatus::Error);
      tx_state_manager_->AddOrUpdateTx(*meta);
      continue;
    }

    // Update SolanaTxMeta with signature status.
    if (!signature_statuses[i]->confirmation_status.empty()) {
      meta->set_signature_status(*signature_statuses[i]);

      if (signature_statuses[i]->confirmation_status == "finalized") {
        meta->set_status(mojom::TransactionStatus::Confirmed);
        meta->set_confirmed_time(base::Time::Now());
      }

      tx_state_manager_->AddOrUpdateTx(*meta);
    }
  }
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

void SolanaTxManager::OnLatestBlockhashUpdated(const std::string& blockhash) {
  UpdatePendingTransactions();
}

SolanaTxStateManager* SolanaTxManager::GetSolanaTxStateManager() {
  return static_cast<SolanaTxStateManager*>(tx_state_manager_.get());
}

SolanaBlockTracker* SolanaTxManager::GetSolanaBlockTracker() {
  return static_cast<SolanaBlockTracker*>(block_tracker_.get());
}

std::unique_ptr<SolanaTxMeta> SolanaTxManager::GetTxForTesting(
    const std::string& tx_meta_id) {
  return GetSolanaTxStateManager()->GetSolanaTx(tx_meta_id);
}

}  // namespace brave_wallet
