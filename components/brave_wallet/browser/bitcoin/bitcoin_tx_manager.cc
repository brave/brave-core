/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_tx_manager.h"

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_block_tracker.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_transaction.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_tx_meta.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

BitcoinTxManager::BitcoinTxManager(
    TxService* tx_service,
    BitcoinWalletService* bitcoin_wallet_service,
    KeyringService* keyring_service,
    PrefService* prefs,
    TxStorageDelegate* delegate,
    AccountResolverDelegate* account_resolver_delegate)
    : TxManager(
          std::make_unique<BitcoinTxStateManager>(prefs,
                                                  delegate,
                                                  account_resolver_delegate),
          std::make_unique<BitcoinBlockTracker>(
              &bitcoin_wallet_service->bitcoin_rpc()),
          tx_service,
          keyring_service,
          prefs),
      bitcoin_wallet_service_(bitcoin_wallet_service) {
  block_tracker_observation_.Observe(GetBitcoinBlockTracker());
}

BitcoinTxManager::~BitcoinTxManager() = default;

std::unique_ptr<BitcoinTxMeta> BitcoinTxManager::GetTxForTesting(
    const std::string& tx_meta_id) {
  return GetBitcoinTxStateManager()->GetBitcoinTx(tx_meta_id);
}

void BitcoinTxManager::GetBtcHardwareTransactionSignData(
    const std::string& tx_meta_id,
    GetBtcHardwareTransactionSignDataCallback callback) {
  std::unique_ptr<BitcoinTxMeta> meta =
      GetBitcoinTxStateManager()->GetBitcoinTx(tx_meta_id);
  if (!meta || !meta->tx()) {
    std::move(callback).Run(nullptr);
    return;
  }

  std::move(callback).Run(
      bitcoin_wallet_service_->GetBtcHardwareTransactionSignData(*meta->tx(),
                                                                 meta->from()));
}

void BitcoinTxManager::ProcessBtcHardwareSignature(
    const std::string& tx_meta_id,
    const mojom::BitcoinSignaturePtr& hw_signature,
    ProcessBtcHardwareSignatureCallback callback) {
  CHECK(hw_signature);
  std::unique_ptr<BitcoinTxMeta> meta =
      GetBitcoinTxStateManager()->GetBitcoinTx(tx_meta_id);
  if (!meta) {
    std::move(callback).Run(false);
    return;
  }

  meta->set_status(mojom::TransactionStatus::Approved);
  if (!tx_state_manager_->AddOrUpdateTx(*meta)) {
    std::move(callback).Run(false);
    return;
  }

  bitcoin_wallet_service_->PostHwSignedTransaction(
      meta->from(), *meta->tx(), *hw_signature,
      base::BindOnce(
          &BitcoinTxManager::ContinueApproveTransaction,
          weak_factory_.GetWeakPtr(), tx_meta_id,
          base::BindOnce(&BitcoinTxManager::ContinueProcessBtcHardwareSignature,
                         weak_factory_.GetWeakPtr(), std::move(callback))));
}

void BitcoinTxManager::ContinueProcessBtcHardwareSignature(
    ProcessBtcHardwareSignatureCallback callback,
    bool success,
    mojom::ProviderErrorUnionPtr error,
    const std::string& message) {
  std::move(callback).Run(success);
}

void BitcoinTxManager::OnLatestHeightUpdated(const std::string& chain_id,
                                             uint32_t latest_height) {
  UpdatePendingTransactions(chain_id);
}

void BitcoinTxManager::AddUnapprovedTransaction(
    const std::string& chain_id,
    mojom::TxDataUnionPtr tx_data_union,
    const mojom::AccountIdPtr& from,
    const std::optional<url::Origin>& origin,
    AddUnapprovedTransactionCallback callback) {
  if (chain_id != GetNetworkForBitcoinAccount(from)) {
    std::move(callback).Run(false, "", WalletInternalErrorMessage());
    return;
  }

  const auto& btc_tx_data = tx_data_union->get_btc_tx_data();

  bitcoin_wallet_service_->CreateTransaction(
      from->Clone(), btc_tx_data->to, btc_tx_data->amount,
      btc_tx_data->sending_max_amount,
      base::BindOnce(&BitcoinTxManager::ContinueAddUnapprovedTransaction,
                     weak_factory_.GetWeakPtr(), chain_id, from.Clone(), origin,
                     std::move(callback)));
}

void BitcoinTxManager::ContinueAddUnapprovedTransaction(
    const std::string& chain_id,
    const mojom::AccountIdPtr& from,
    const std::optional<url::Origin>& origin,
    AddUnapprovedTransactionCallback callback,
    base::expected<BitcoinTransaction, std::string> bitcoin_transaction) {
  if (!bitcoin_transaction.has_value()) {
    std::move(callback).Run(false, "", bitcoin_transaction.error());
    return;
  }

  BitcoinTxMeta meta(from, std::make_unique<BitcoinTransaction>(
                               std::move(bitcoin_transaction.value())));
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_origin(
      origin.value_or(url::Origin::Create(GURL("chrome://wallet"))));
  meta.set_created_time(base::Time::Now());
  meta.set_status(mojom::TransactionStatus::Unapproved);
  meta.set_chain_id(chain_id);

  if (!tx_state_manager_->AddOrUpdateTx(meta)) {
    std::move(callback).Run(false, "", WalletInternalErrorMessage());
    return;
  }
  std::move(callback).Run(true, meta.id(), "");
}

void BitcoinTxManager::ApproveTransaction(const std::string& tx_meta_id,
                                          ApproveTransactionCallback callback) {
  std::unique_ptr<BitcoinTxMeta> meta =
      GetBitcoinTxStateManager()->GetBitcoinTx(tx_meta_id);
  if (!meta) {
    DCHECK(false) << "Transaction should be found";
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewBitcoinProviderError(
            mojom::BitcoinProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  meta->set_status(mojom::TransactionStatus::Approved);
  if (!tx_state_manager_->AddOrUpdateTx(*meta)) {
    std::move(callback).Run(false,
                            mojom::ProviderErrorUnion::NewBitcoinProviderError(
                                mojom::BitcoinProviderError::kInternalError),
                            WalletInternalErrorMessage());
    return;
  }

  bitcoin_wallet_service_->SignAndPostTransaction(
      meta->from(), std::move(*meta->tx()),
      base::BindOnce(&BitcoinTxManager::ContinueApproveTransaction,
                     weak_factory_.GetWeakPtr(), tx_meta_id,
                     std::move(callback)));
}

void BitcoinTxManager::ContinueApproveTransaction(
    const std::string& tx_meta_id,
    ApproveTransactionCallback callback,
    std::string tx_cid,
    BitcoinTransaction transaction,
    std::string error) {
  std::unique_ptr<BitcoinTxMeta> meta =
      GetBitcoinTxStateManager()->GetBitcoinTx(tx_meta_id);
  if (!meta) {
    DCHECK(false) << "Transaction should be found";
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewBitcoinProviderError(
            mojom::BitcoinProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  const bool success = error.empty();
  if (success) {
    meta->set_status(mojom::TransactionStatus::Submitted);
    meta->set_submitted_time(base::Time::Now());
    meta->set_tx_hash(tx_cid);
    meta->set_tx(std::make_unique<BitcoinTransaction>(std::move(transaction)));
  } else {
    meta->set_status(mojom::TransactionStatus::Error);
  }

  if (!tx_state_manager_->AddOrUpdateTx(*meta)) {
    std::move(callback).Run(false,
                            mojom::ProviderErrorUnion::NewBitcoinProviderError(
                                mojom::BitcoinProviderError::kInternalError),
                            WalletInternalErrorMessage());
    return;
  }

  if (success) {
    UpdatePendingTransactions(meta->chain_id());
  }
  std::move(callback).Run(
      success,
      mojom::ProviderErrorUnion::NewBitcoinProviderError(
          success ? mojom::BitcoinProviderError::kSuccess
                  : mojom::BitcoinProviderError::kInternalError),
      error);
}

void BitcoinTxManager::SpeedupOrCancelTransaction(
    const std::string& tx_meta_id,
    bool cancel,
    SpeedupOrCancelTransactionCallback callback) {
  NOTIMPLEMENTED() << "Bitcoin transaction speedup or cancel is not supported";
}

void BitcoinTxManager::RetryTransaction(const std::string& tx_meta_id,
                                        RetryTransactionCallback callback) {
  NOTIMPLEMENTED() << "Bitcoin transaction retry is not supported";
}

BitcoinTxStateManager* BitcoinTxManager::GetBitcoinTxStateManager() {
  return static_cast<BitcoinTxStateManager*>(tx_state_manager_.get());
}

BitcoinBlockTracker* BitcoinTxManager::GetBitcoinBlockTracker() {
  return static_cast<BitcoinBlockTracker*>(block_tracker_.get());
}

mojom::CoinType BitcoinTxManager::GetCoinType() const {
  return mojom::CoinType::BTC;
}

void BitcoinTxManager::UpdatePendingTransactions(
    const std::optional<std::string>& chain_id) {
  auto pending_transactions = tx_state_manager_->GetTransactionsByStatus(
      chain_id, mojom::TransactionStatus::Submitted, std::nullopt);
  std::set<std::string> pending_chain_ids;
  for (const auto& pending_transaction : pending_transactions) {
    auto pending_chain_id = pending_transaction->chain_id();
    bitcoin_wallet_service_->GetTransactionStatus(
        pending_transaction->chain_id(), pending_transaction->tx_hash(),
        base::BindOnce(&BitcoinTxManager::OnGetTransactionStatus,
                       weak_factory_.GetWeakPtr(), pending_transaction->id()));
    pending_chain_ids.emplace(pending_chain_id);
  }
  CheckIfBlockTrackerShouldRun(pending_chain_ids);
}

void BitcoinTxManager::OnGetTransactionStatus(
    const std::string& tx_meta_id,
    base::expected<bool, std::string> confirm_status) {
  if (!confirm_status.has_value()) {
    return;
  }
  std::unique_ptr<BitcoinTxMeta> meta =
      GetBitcoinTxStateManager()->GetBitcoinTx(tx_meta_id);
  if (!meta) {
    return;
  }
  if (confirm_status.value()) {
    // TODO(apaymyshev): dropped and error state.
    mojom::TransactionStatus status = mojom::TransactionStatus::Confirmed;
    meta->set_status(status);
    meta->set_confirmed_time(base::Time::Now());
    tx_state_manager_->AddOrUpdateTx(*meta);
  }
}

}  // namespace brave_wallet
