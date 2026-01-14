/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_tx_manager.h"

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/notimplemented.h"
#include "base/notreached.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_block_tracker.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_tx_meta.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

CardanoTxManager::CardanoTxManager(
    TxService& tx_service,
    CardanoWalletService& cardano_wallet_service,
    KeyringService& keyring_service,
    TxStorageDelegate& delegate,
    AccountResolverDelegate& account_resolver_delegate)
    : TxManager(
          std::make_unique<CardanoTxStateManager>(delegate,
                                                  account_resolver_delegate),
          std::make_unique<CardanoBlockTracker>(cardano_wallet_service),
          tx_service,
          keyring_service),
      cardano_wallet_service_(cardano_wallet_service) {
  block_tracker_observation_.Observe(&GetCardanoBlockTracker());
}

CardanoTxManager::~CardanoTxManager() = default;

std::unique_ptr<CardanoTxMeta> CardanoTxManager::GetTxForTesting(
    const std::string& tx_meta_id) {
  return GetCardanoTxStateManager().GetCardanoTx(tx_meta_id);
}

void CardanoTxManager::AddUnapprovedCardanoTransaction(
    mojom::NewCardanoTransactionParamsPtr params,
    AddUnapprovedCardanoTransactionCallback callback) {
  auto chain_id = params->chain_id;
  if (chain_id != GetNetworkForCardanoAccount(params->from)) {
    std::move(callback).Run(false, "", WalletInternalErrorMessage());
    return;
  }

  auto address_to = CardanoAddress::FromString(params->to);
  if (!address_to) {
    std::move(callback).Run(false, "", WalletInternalErrorMessage());
    return;
  }

  // We don't support Cardano dApps so far, so all transactions come from
  // wallet origin.
  std::optional<url::Origin> origin = std::nullopt;

  cardano_wallet_service_->CreateCardanoTransaction(
      params->from.Clone(), *address_to, params->amount,
      params->sending_max_amount,
      base::BindOnce(&CardanoTxManager::ContinueAddUnapprovedTransaction,
                     weak_factory_.GetWeakPtr(), chain_id, params->from.Clone(),
                     origin, std::move(params->swap_info),
                     std::move(callback)));
}

void CardanoTxManager::OnLatestHeightUpdated(const std::string& chain_id,
                                             uint32_t latest_height) {
  UpdatePendingTransactions(chain_id);
}

void CardanoTxManager::AddUnapprovedTransaction(
    const std::string& chain_id,
    mojom::TxDataUnionPtr tx_data_union,
    const mojom::AccountIdPtr& from,
    const std::optional<url::Origin>& origin,
    mojom::SwapInfoPtr swap_info,
    AddUnapprovedTransactionCallback callback) {
  NOTREACHED() << "AddUnapprovedCardanoTransaction must be used";
}

void CardanoTxManager::ContinueAddUnapprovedTransaction(
    const std::string& chain_id,
    const mojom::AccountIdPtr& from,
    const std::optional<url::Origin>& origin,
    mojom::SwapInfoPtr swap_info,
    AddUnapprovedTransactionCallback callback,
    base::expected<CardanoTransaction, std::string> cardano_transaction) {
  if (!cardano_transaction.has_value()) {
    std::move(callback).Run(false, "", cardano_transaction.error());
    return;
  }

  CardanoTxMeta meta(from, std::make_unique<CardanoTransaction>(
                               std::move(cardano_transaction.value())));
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_origin(
      origin.value_or(url::Origin::Create(GURL("chrome://wallet"))));
  meta.set_created_time(base::Time::Now());
  meta.set_status(mojom::TransactionStatus::Unapproved);
  meta.set_chain_id(chain_id);
  meta.set_swap_info(std::move(swap_info));

  if (!tx_state_manager().AddOrUpdateTx(meta)) {
    std::move(callback).Run(false, "", WalletInternalErrorMessage());
    return;
  }
  std::move(callback).Run(true, meta.id(), "");
}

void CardanoTxManager::ApproveTransaction(const std::string& tx_meta_id,
                                          ApproveTransactionCallback callback) {
  std::unique_ptr<CardanoTxMeta> meta =
      GetCardanoTxStateManager().GetCardanoTx(tx_meta_id);
  if (!meta) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewCardanoProviderError(
            mojom::CardanoProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  meta->set_status(mojom::TransactionStatus::Approved);
  if (!tx_state_manager().AddOrUpdateTx(*meta)) {
    std::move(callback).Run(false,
                            mojom::ProviderErrorUnion::NewCardanoProviderError(
                                mojom::CardanoProviderError::kInternalError),
                            WalletInternalErrorMessage());
    return;
  }

  cardano_wallet_service_->SignAndPostTransaction(
      meta->from(), std::move(*meta->tx()),
      base::BindOnce(&CardanoTxManager::ContinueApproveTransaction,
                     weak_factory_.GetWeakPtr(), tx_meta_id,
                     std::move(callback)));
}

void CardanoTxManager::ContinueApproveTransaction(
    const std::string& tx_meta_id,
    ApproveTransactionCallback callback,
    std::string tx_cid,
    CardanoTransaction transaction,
    std::string error) {
  std::unique_ptr<CardanoTxMeta> meta =
      GetCardanoTxStateManager().GetCardanoTx(tx_meta_id);
  if (!meta) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewCardanoProviderError(
            mojom::CardanoProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  const bool success = error.empty();
  if (success) {
    meta->set_status(mojom::TransactionStatus::Submitted);
    meta->set_submitted_time(base::Time::Now());
    meta->set_tx_hash(tx_cid);
    meta->set_tx(std::make_unique<CardanoTransaction>(std::move(transaction)));
  } else {
    meta->set_status(mojom::TransactionStatus::Error);
  }

  if (!tx_state_manager().AddOrUpdateTx(*meta)) {
    std::move(callback).Run(false,
                            mojom::ProviderErrorUnion::NewCardanoProviderError(
                                mojom::CardanoProviderError::kInternalError),
                            WalletInternalErrorMessage());
    return;
  }

  if (success) {
    UpdatePendingTransactions(meta->chain_id());
  }
  std::move(callback).Run(
      success,
      mojom::ProviderErrorUnion::NewCardanoProviderError(
          success ? mojom::CardanoProviderError::kSuccess
                  : mojom::CardanoProviderError::kInternalError),
      error);
}

void CardanoTxManager::SpeedupOrCancelTransaction(
    const std::string& tx_meta_id,
    bool cancel,
    SpeedupOrCancelTransactionCallback callback) {
  NOTIMPLEMENTED() << "Cardano transaction speedup or cancel is not supported";
}

void CardanoTxManager::RetryTransaction(const std::string& tx_meta_id,
                                        RetryTransactionCallback callback) {
  NOTIMPLEMENTED() << "Cardano transaction retry is not supported";
}

CardanoTxStateManager& CardanoTxManager::GetCardanoTxStateManager() {
  return static_cast<CardanoTxStateManager&>(tx_state_manager());
}

CardanoBlockTracker& CardanoTxManager::GetCardanoBlockTracker() {
  return static_cast<CardanoBlockTracker&>(block_tracker());
}

mojom::CoinType CardanoTxManager::GetCoinType() const {
  return mojom::CoinType::ADA;
}

void CardanoTxManager::UpdatePendingTransactions(
    const std::optional<std::string>& chain_id) {
  auto pending_transactions = tx_state_manager().GetTransactionsByStatus(
      chain_id, mojom::TransactionStatus::Submitted, std::nullopt);
  std::set<std::string> pending_chain_ids;
  for (const auto& pending_transaction : pending_transactions) {
    auto pending_chain_id = pending_transaction->chain_id();
    cardano_wallet_service_->GetTransactionStatus(
        pending_transaction->chain_id(), pending_transaction->tx_hash(),
        base::BindOnce(&CardanoTxManager::OnGetTransactionStatus,
                       weak_factory_.GetWeakPtr(), pending_transaction->id()));
    pending_chain_ids.emplace(pending_chain_id);
  }
  CheckIfBlockTrackerShouldRun(pending_chain_ids);
}

void CardanoTxManager::OnGetTransactionStatus(
    const std::string& tx_meta_id,
    base::expected<bool, std::string> confirm_status) {
  if (!confirm_status.has_value()) {
    return;
  }
  std::unique_ptr<CardanoTxMeta> meta =
      GetCardanoTxStateManager().GetCardanoTx(tx_meta_id);
  if (!meta) {
    return;
  }
  if (confirm_status.value()) {
    mojom::TransactionStatus status = mojom::TransactionStatus::Confirmed;
    meta->set_status(status);
    meta->set_confirmed_time(base::Time::Now());
    tx_state_manager().AddOrUpdateTx(*meta);
  }
}

}  // namespace brave_wallet
