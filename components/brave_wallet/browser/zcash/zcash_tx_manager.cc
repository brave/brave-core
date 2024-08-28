/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_tx_manager.h"

#include <memory>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/notreached.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_block_tracker.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_tx_meta.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

ZCashTxManager::ZCashTxManager(
    TxService* tx_service,
    ZCashWalletService* zcash_wallet_service,
    KeyringService* keyring_service,
    PrefService* prefs,
    TxStorageDelegate* delegate,
    AccountResolverDelegate* account_resolver_delegate)
    : TxManager(
          std::make_unique<ZCashTxStateManager>(prefs,
                                                delegate,
                                                account_resolver_delegate),
          std::make_unique<ZCashBlockTracker>(
              zcash_wallet_service->zcash_rpc()),
          tx_service,
          keyring_service,
          prefs),
      zcash_wallet_service_(zcash_wallet_service),
      zcash_rpc_(zcash_wallet_service->zcash_rpc()) {
  block_tracker_observation_.Observe(GetZCashBlockTracker());
}

ZCashTxManager::~ZCashTxManager() = default;

void ZCashTxManager::OnLatestHeightUpdated(const std::string& chain_id,
                                           uint32_t latest_height) {
  UpdatePendingTransactions(chain_id);
}

void ZCashTxManager::AddUnapprovedTransaction(
    const std::string& chain_id,
    mojom::TxDataUnionPtr tx_data_union,
    const mojom::AccountIdPtr& from,
    const std::optional<url::Origin>& origin,
    AddUnapprovedTransactionCallback callback) {
  const auto& zec_tx_data = tx_data_union->get_zec_tx_data();

#if BUILDFLAG(ENABLE_ORCHARD)
  if (IsZCashShieldedTransactionsEnabled()) {
    bool has_orchard_part =
        ExtractOrchardPart(zec_tx_data->to, chain_id == mojom::kZCashTestnet)
            .has_value();
    if (has_orchard_part) {
      std::optional<OrchardMemo> memo = ToOrchardMemo(zec_tx_data->memo);
      if (!memo && zec_tx_data->memo) {
        std::move(callback).Run(false, "", "");
        return;
      }
      zcash_wallet_service_->CreateShieldTransaction(
          chain_id, from->Clone(), zec_tx_data->to, zec_tx_data->amount, memo,
          base::BindOnce(&ZCashTxManager::ContinueAddUnapprovedTransaction,
                         weak_factory_.GetWeakPtr(), chain_id, from.Clone(),
                         origin, std::move(callback)));
      return;
    }
  }
#endif
  zcash_wallet_service_->CreateFullyTransparentTransaction(
      chain_id, from->Clone(), zec_tx_data->to, zec_tx_data->amount,
      base::BindOnce(&ZCashTxManager::ContinueAddUnapprovedTransaction,
                     weak_factory_.GetWeakPtr(), chain_id, from.Clone(), origin,
                     std::move(callback)));
}

void ZCashTxManager::ContinueAddUnapprovedTransaction(
    const std::string& chain_id,
    const mojom::AccountIdPtr& from,
    const std::optional<url::Origin>& origin,
    AddUnapprovedTransactionCallback callback,
    base::expected<ZCashTransaction, std::string> zcash_transaction) {
  if (!zcash_transaction.has_value()) {
    std::move(callback).Run(false, "", zcash_transaction.error());
    return;
  }

  ZCashTxMeta meta(from, std::make_unique<ZCashTransaction>(
                             std::move(zcash_transaction.value())));
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_origin(
      origin.value_or(url::Origin::Create(GURL("chrome://wallet"))));
  meta.set_created_time(base::Time::Now());
  meta.set_status(mojom::TransactionStatus::Unapproved);
  meta.set_chain_id(chain_id);
  if (!tx_state_manager_->AddOrUpdateTx(meta)) {
    std::move(callback).Run(
        false, "", l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  std::move(callback).Run(true, meta.id(), "");
}

void ZCashTxManager::ApproveTransaction(const std::string& tx_meta_id,
                                        ApproveTransactionCallback callback) {
  std::unique_ptr<ZCashTxMeta> meta =
      GetZCashTxStateManager()->GetZCashTx(tx_meta_id);
  if (!meta) {
    DCHECK(false) << "Transaction should be found";
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewZcashProviderError(
            mojom::ZCashProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  // Only one transaction per account is allowed at one moment.
  if (!GetZCashTxStateManager()
           ->GetTransactionsByStatus(meta->chain_id(),
                                     mojom::TransactionStatus::Submitted,
                                     meta->from())
           .empty()) {
    meta->set_status(mojom::TransactionStatus::Error);
    tx_state_manager_->AddOrUpdateTx(*meta);

    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewZcashProviderError(
            mojom::ZCashProviderError::kMultipleTransactionsNotSupported),
        l10n_util::GetStringUTF8(
            IDS_BRAVE_WALLET_ZCASH_TRANSACTION_ALREADY_EXISTS_DESCRIPTION));
    return;
  }

  meta->set_status(mojom::TransactionStatus::Approved);
  if (!tx_state_manager_->AddOrUpdateTx(*meta)) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewZcashProviderError(
            mojom::ZCashProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  zcash_wallet_service_->SignAndPostTransaction(
      meta->chain_id(), meta->from(), std::move(*meta->tx()),
      base::BindOnce(&ZCashTxManager::ContinueApproveTransaction,
                     weak_factory_.GetWeakPtr(), tx_meta_id,
                     std::move(callback)));
}

void ZCashTxManager::ContinueApproveTransaction(
    const std::string& tx_meta_id,
    ApproveTransactionCallback callback,
    std::string tx_cid,
    ZCashTransaction transaction,
    std::string error) {
  std::unique_ptr<ZCashTxMeta> meta =
      GetZCashTxStateManager()->GetZCashTx(tx_meta_id);
  if (!meta) {
    DCHECK(false) << "Transaction should be found";
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewZcashProviderError(
            mojom::ZCashProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  const bool success = error.empty();
  if (success) {
    meta->set_status(mojom::TransactionStatus::Submitted);
    meta->set_submitted_time(base::Time::Now());
    meta->set_tx_hash(tx_cid);
    meta->set_tx(std::make_unique<ZCashTransaction>(std::move(transaction)));
  } else {
    meta->set_status(mojom::TransactionStatus::Error);
  }

  if (!tx_state_manager_->AddOrUpdateTx(*meta)) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewZcashProviderError(
            mojom::ZCashProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  if (success) {
    UpdatePendingTransactions(meta->chain_id());
  }
  std::move(callback).Run(
      success,
      mojom::ProviderErrorUnion::NewZcashProviderError(
          success ? mojom::ZCashProviderError::kSuccess
                  : mojom::ZCashProviderError::kInternalError),
      error);
}

void ZCashTxManager::SpeedupOrCancelTransaction(
    const std::string& tx_meta_id,
    bool cancel,
    SpeedupOrCancelTransactionCallback callback) {
  NOTIMPLEMENTED() << "Zcash transaction speedup or cancel is not supported";
}

void ZCashTxManager::RetryTransaction(const std::string& tx_meta_id,
                                      RetryTransactionCallback callback) {
  NOTIMPLEMENTED() << "Zcash transaction retry is not supported";
}

ZCashTxStateManager* ZCashTxManager::GetZCashTxStateManager() {
  return static_cast<ZCashTxStateManager*>(tx_state_manager_.get());
}

ZCashBlockTracker* ZCashTxManager::GetZCashBlockTracker() {
  return static_cast<ZCashBlockTracker*>(block_tracker_.get());
}

mojom::CoinType ZCashTxManager::GetCoinType() const {
  return mojom::CoinType::ZEC;
}

void ZCashTxManager::UpdatePendingTransactions(
    const std::optional<std::string>& chain_id) {
  auto pending_transactions = tx_state_manager_->GetTransactionsByStatus(
      chain_id, mojom::TransactionStatus::Submitted, std::nullopt);
  std::set<std::string> pending_chain_ids;
  for (const auto& pending_transaction : pending_transactions) {
    auto pending_chain_id = pending_transaction->chain_id();
    zcash_wallet_service_->GetTransactionStatus(
        pending_transaction->chain_id(), pending_transaction->tx_hash(),
        base::BindOnce(&ZCashTxManager::OnGetTransactionStatus,
                       weak_factory_.GetWeakPtr(), pending_transaction->id()));
    pending_chain_ids.emplace(pending_chain_id);
  }
  CheckIfBlockTrackerShouldRun(pending_chain_ids);
}

void ZCashTxManager::OnGetTransactionStatus(
    const std::string& tx_meta_id,
    base::expected<bool, std::string> confirm_status) {
  if (!confirm_status.has_value()) {
    return;
  }
  std::unique_ptr<ZCashTxMeta> meta =
      GetZCashTxStateManager()->GetZCashTx(tx_meta_id);
  if (!meta) {
    return;
  }
  if (confirm_status.value()) {
    // TODO(cypt4): dropped and error state.
    mojom::TransactionStatus status = mojom::TransactionStatus::Confirmed;
    meta->set_status(status);
    meta->set_confirmed_time(base::Time::Now());
    tx_state_manager_->AddOrUpdateTx(*meta);
  }
}

}  // namespace brave_wallet
