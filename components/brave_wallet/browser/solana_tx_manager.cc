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
#include "brave/components/brave_wallet/browser/solana_instruction_builder.h"
#include "brave/components/brave_wallet/browser/solana_keyring.h"
#include "brave/components/brave_wallet/browser/solana_tx_meta.h"
#include "brave/components/brave_wallet/browser/solana_tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
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

  GetSolanaBlockTracker()->GetLatestBlockhash(
      base::BindOnce(&SolanaTxManager::OnGetLatestBlockhash,
                     weak_ptr_factory_.GetWeakPtr(), std::move(meta),
                     std::move(callback)),
      true);
}

void SolanaTxManager::OnGetLatestBlockhash(std::unique_ptr<SolanaTxMeta> meta,
                                           ApproveTransactionCallback callback,
                                           const std::string& latest_blockhash,
                                           uint64_t last_valid_block_height,
                                           mojom::SolanaProviderError error,
                                           const std::string& error_message) {
  if (error != mojom::SolanaProviderError::kSuccess) {
    std::move(callback).Run(
        false, mojom::ProviderErrorUnion::NewSolanaProviderError(error),
        error_message);
    return;
  }

  meta->set_status(mojom::TransactionStatus::Approved);
  meta->tx()->message()->set_recent_blockhash(latest_blockhash);
  meta->tx()->message()->set_last_valid_block_height(last_valid_block_height);
  tx_state_manager_->AddOrUpdateTx(*meta);

  json_rpc_service_->SendSolanaTransaction(
      meta->tx()->GetSignedTransaction(keyring_service_),
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
  json_rpc_service_->GetSolanaBlockHeight(base::BindOnce(
      &SolanaTxManager::OnGetBlockHeight, weak_ptr_factory_.GetWeakPtr()));
}

void SolanaTxManager::OnGetBlockHeight(uint64_t block_height,
                                       mojom::SolanaProviderError error,
                                       const std::string& error_message) {
  if (error != mojom::SolanaProviderError::kSuccess)
    return;

  auto pending_transactions = tx_state_manager_->GetTransactionsByStatus(
      mojom::TransactionStatus::Submitted, absl::nullopt);
  std::vector<std::string> tx_meta_ids;
  std::vector<std::string> tx_signatures;
  for (const auto& pending_transaction : pending_transactions) {
    tx_meta_ids.push_back(pending_transaction->id());
    tx_signatures.push_back(pending_transaction->tx_hash());
  }
  json_rpc_service_->GetSolanaSignatureStatuses(
      tx_signatures, base::BindOnce(&SolanaTxManager::OnGetSignatureStatuses,
                                    weak_ptr_factory_.GetWeakPtr(), tx_meta_ids,
                                    block_height));
  known_no_pending_tx_ = pending_transactions.empty();
  CheckIfBlockTrackerShouldRun();
}

void SolanaTxManager::OnGetSignatureStatuses(
    const std::vector<std::string>& tx_meta_ids,
    uint64_t block_height,
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
    if (!meta)
      continue;

    if (!signature_statuses[i]) {
      if (meta->tx()->message()->last_valid_block_height() < block_height) {
        meta->set_status(mojom::TransactionStatus::Dropped);
        tx_state_manager_->AddOrUpdateTx(*meta);
      }
      continue;
    }

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

void SolanaTxManager::MakeSystemProgramTransferTxData(
    const std::string& from,
    const std::string& to,
    uint64_t lamports,
    MakeSystemProgramTransferTxDataCallback callback) {
  absl::optional<SolanaInstruction> instruction =
      solana::system_program::Transfer(from, to, lamports);
  if (!instruction) {
    std::move(callback).Run(
        nullptr, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  // recent_blockhash will be updated when we are going to send out the tx.
  SolanaTransaction transaction("" /* recent_blockhash*/, 0, from,
                                {*instruction});
  transaction.set_to_wallet_address(to);
  transaction.set_tx_type(mojom::TransactionType::SolanaSystemTransfer);
  transaction.set_lamports(lamports);

  auto tx_data = transaction.ToSolanaTxData();
  // This won't be null because we will always construct the mojo struct.
  DCHECK(tx_data);
  std::move(callback).Run(std::move(tx_data),
                          mojom::SolanaProviderError::kSuccess, "");
}

void SolanaTxManager::MakeTokenProgramTransferTxData(
    const std::string& spl_token_mint_address,
    const std::string& from_wallet_address,
    const std::string& to_wallet_address,
    uint64_t amount,
    MakeTokenProgramTransferTxDataCallback callback) {
  absl::optional<std::string> from_associated_token_account =
      SolanaKeyring::GetAssociatedTokenAccount(spl_token_mint_address,
                                               from_wallet_address);
  absl::optional<std::string> to_associated_token_account =
      SolanaKeyring::GetAssociatedTokenAccount(spl_token_mint_address,
                                               to_wallet_address);
  if (!from_associated_token_account || !to_associated_token_account) {
    std::move(callback).Run(
        nullptr, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  // Check if the receiver's associated token account is existed or not.
  json_rpc_service_->GetSolanaAccountInfo(
      *to_associated_token_account,
      base::BindOnce(
          &SolanaTxManager::OnGetAccountInfo, weak_ptr_factory_.GetWeakPtr(),
          spl_token_mint_address, from_wallet_address, to_wallet_address,
          *from_associated_token_account, *to_associated_token_account, amount,
          std::move(callback)));
}

void SolanaTxManager::OnGetAccountInfo(
    const std::string& spl_token_mint_address,
    const std::string& from_wallet_address,
    const std::string& to_wallet_address,
    const std::string& from_associated_token_account,
    const std::string& to_associated_token_account,
    uint64_t amount,
    MakeTokenProgramTransferTxDataCallback callback,
    absl::optional<SolanaAccountInfo> account_info,
    mojom::SolanaProviderError error,
    const std::string& error_message) {
  if (error != mojom::SolanaProviderError::kSuccess) {
    std::move(callback).Run(nullptr, error, error_message);
    return;
  }

  bool create_associated_token_account = false;
  std::vector<SolanaInstruction> instructions;
  if (!account_info || account_info->owner != kSolanaTokenProgramId) {
    absl::optional<SolanaInstruction> create_associated_token_instruction =
        solana::spl_associated_token_account_program::
            CreateAssociatedTokenAccount(from_wallet_address, to_wallet_address,
                                         to_associated_token_account,
                                         spl_token_mint_address);
    if (!create_associated_token_instruction) {
      std::move(callback).Run(
          nullptr, mojom::SolanaProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
      return;
    }
    instructions.push_back(std::move(*create_associated_token_instruction));
    create_associated_token_account = true;
  }

  absl::optional<SolanaInstruction> transfer_instruction =
      solana::spl_token_program::Transfer(
          kSolanaTokenProgramId, from_associated_token_account,
          to_associated_token_account, from_wallet_address,
          std::vector<std::string>(), amount);
  if (!transfer_instruction) {
    std::move(callback).Run(
        nullptr, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  instructions.push_back(std::move(*transfer_instruction));

  // recent_blockhash will be updated when we are going to send out the tx.
  SolanaTransaction transaction("" /* recent_blockhash*/, 0,
                                from_wallet_address, std::move(instructions));
  transaction.set_to_wallet_address(to_wallet_address);
  transaction.set_spl_token_mint_address(spl_token_mint_address);
  transaction.set_amount(amount);
  transaction.set_tx_type(
      create_associated_token_account
          ? mojom::TransactionType::
                SolanaSPLTokenTransferWithAssociatedTokenAccountCreation
          : mojom::TransactionType::SolanaSPLTokenTransfer);

  auto tx_data = transaction.ToSolanaTxData();
  // This won't be null because we will always construct the mojo struct.
  DCHECK(tx_data);
  std::move(callback).Run(std::move(tx_data),
                          mojom::SolanaProviderError::kSuccess, "");
}

void SolanaTxManager::GetEstimatedTxFee(const std::string& tx_meta_id,
                                        GetEstimatedTxFeeCallback callback) {
  std::unique_ptr<SolanaTxMeta> meta =
      GetSolanaTxStateManager()->GetSolanaTx(tx_meta_id);
  if (!meta) {
    DCHECK(false) << "Transaction should be found";
    std::move(callback).Run(
        false, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  GetSolanaBlockTracker()->GetLatestBlockhash(
      base::BindOnce(&SolanaTxManager::OnGetLatestBlockhashForGetEstimatedTxFee,
                     weak_ptr_factory_.GetWeakPtr(), std::move(meta),
                     std::move(callback)),
      true);
}

void SolanaTxManager::OnGetLatestBlockhashForGetEstimatedTxFee(
    std::unique_ptr<SolanaTxMeta> meta,
    GetEstimatedTxFeeCallback callback,
    const std::string& latest_blockhash,
    uint64_t last_valid_block_height,
    mojom::SolanaProviderError error,
    const std::string& error_message) {
  if (error != mojom::SolanaProviderError::kSuccess) {
    std::move(callback).Run(0, error, error_message);
    return;
  }

  meta->tx()->message()->set_recent_blockhash(latest_blockhash);
  meta->tx()->message()->set_last_valid_block_height(last_valid_block_height);
  const std::string base64_encoded_message =
      meta->tx()->GetBase64EncodedMessage();
  json_rpc_service_->GetSolanaFeeForMessage(
      base64_encoded_message,
      base::BindOnce(&SolanaTxManager::OnGetFeeForMessage,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void SolanaTxManager::OnGetFeeForMessage(GetEstimatedTxFeeCallback callback,
                                         uint64_t tx_fee,
                                         mojom::SolanaProviderError error,
                                         const std::string& error_message) {
  std::move(callback).Run(tx_fee, error, error_message);
}

void SolanaTxManager::OnLatestBlockhashUpdated(
    const std::string& blockhash,
    uint64_t last_valid_block_height) {
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
