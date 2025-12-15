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
#include "base/logging.h"
#include "base/notimplemented.h"
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
    TxService& tx_service,
    ZCashWalletService& zcash_wallet_service,
    KeyringService& keyring_service,
    TxStorageDelegate& delegate,
    AccountResolverDelegate& account_resolver_delegate)
    : TxManager(
          std::make_unique<ZCashTxStateManager>(delegate,
                                                account_resolver_delegate),
          std::make_unique<ZCashBlockTracker>(zcash_wallet_service.zcash_rpc()),
          tx_service,
          keyring_service),
      zcash_wallet_service_(zcash_wallet_service) {
  block_tracker_observation_.Observe(&GetZCashBlockTracker());
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
  NOTREACHED() << "AddUnapprovedZCashTransaction must be used";
}

void ZCashTxManager::AddUnapprovedZCashTransaction(
    mojom::NewZCashTransactionParamsPtr params,
    AddUnapprovedZCashTransactionCallback callback) {
  auto from = params->from.Clone();
  auto chain_id = params->chain_id;
  auto tx_result = zcash_wallet_service_->GetTransactionType(
      from, params->use_shielded_pool, params->to);
  if (!tx_result.has_value()) {
    std::move(callback).Run(false, "", "");
    return;
  }

  uint64_t amount =
      params->sending_max_amount ? kZCashFullAmount : params->amount;

  // We don't support ZCash dApps so far, so all transactions come from
  // wallet origin.
  std::optional<url::Origin> origin = std::nullopt;

#if BUILDFLAG(ENABLE_ORCHARD)
  if (IsZCashShieldedTransactionsEnabled()) {
    std::optional<OrchardMemo> memo = ToOrchardMemo(params->memo);
    if (!memo && params->memo) {
      std::move(callback).Run(false, "", "");
      return;
    }
    if (tx_result.value() == mojom::ZCashTxType::kOrchardToOrchard) {
      zcash_wallet_service_->CreateOrchardToOrchardTransaction(
          from->Clone(), params->to, amount, memo,
          base::BindOnce(&ZCashTxManager::ContinueAddUnapprovedTransaction,
                         weak_factory_.GetWeakPtr(), from.Clone(), origin,
                         std::move(callback)));
      return;
    } else if (tx_result.value() == mojom::ZCashTxType::kTransparentToOrchard ||
               tx_result.value() == mojom::ZCashTxType::kShielding) {
      zcash_wallet_service_->CreateTransparentToOrchardTransaction(
          from->Clone(), params->to, amount, memo,
          base::BindOnce(&ZCashTxManager::ContinueAddUnapprovedTransaction,
                         weak_factory_.GetWeakPtr(), from.Clone(), origin,
                         std::move(callback)));
      return;
    } else if (tx_result.value() == mojom::ZCashTxType::kOrchardToTransparent) {
      zcash_wallet_service_->CreateOrchardToTransparentTransaction(
          from->Clone(), params->to, amount,
          base::BindOnce(&ZCashTxManager::ContinueAddUnapprovedTransaction,
                         weak_factory_.GetWeakPtr(), from.Clone(), origin,
                         std::move(callback)));
      return;
    }
  }
#endif
  if (tx_result.value() == mojom::ZCashTxType::kTransparentToTransparent) {
    zcash_wallet_service_->CreateFullyTransparentTransaction(
        from->Clone(), params->to, amount,
        base::BindOnce(&ZCashTxManager::ContinueAddUnapprovedTransaction,
                       weak_factory_.GetWeakPtr(), from.Clone(), origin,
                       std::move(callback)));
    return;
  }

  std::move(callback).Run(false, "", "");
}

void ZCashTxManager::ContinueAddUnapprovedTransaction(
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
  meta.set_chain_id(GetNetworkForZCashAccount(from));
  if (!tx_state_manager().AddOrUpdateTx(meta)) {
    std::move(callback).Run(
        false, "", l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  std::move(callback).Run(true, meta.id(), "");
}

void ZCashTxManager::ApproveTransaction(const std::string& tx_meta_id,
                                        ApproveTransactionCallback callback) {
  std::unique_ptr<ZCashTxMeta> meta =
      GetZCashTxStateManager().GetZCashTx(tx_meta_id);
  if (!meta) {
    LOG(ERROR) << "Transaction should be found";
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewZcashProviderError(
            mojom::ZCashProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  // Only one transaction per account is allowed at one moment.
  if (!GetZCashTxStateManager()
           .GetTransactionsByStatus(meta->chain_id(),
                                    mojom::TransactionStatus::Submitted,
                                    meta->from())
           .empty()) {
    meta->set_status(mojom::TransactionStatus::Error);
    tx_state_manager().AddOrUpdateTx(*meta);

    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewZcashProviderError(
            mojom::ZCashProviderError::kMultipleTransactionsNotSupported),
        l10n_util::GetStringUTF8(
            IDS_BRAVE_WALLET_ZCASH_TRANSACTION_ALREADY_EXISTS_DESCRIPTION));
    return;
  }

  meta->set_status(mojom::TransactionStatus::Approved);
  if (!tx_state_manager().AddOrUpdateTx(*meta)) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewZcashProviderError(
            mojom::ZCashProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  zcash_wallet_service_->SignAndPostTransaction(
      meta->from(), std::move(*meta->tx()),
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
      GetZCashTxStateManager().GetZCashTx(tx_meta_id);
  if (!meta) {
    LOG(ERROR) << "Transaction should be found";
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

  if (!tx_state_manager().AddOrUpdateTx(*meta)) {
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

ZCashTxStateManager& ZCashTxManager::GetZCashTxStateManager() {
  return static_cast<ZCashTxStateManager&>(tx_state_manager());
}

ZCashBlockTracker& ZCashTxManager::GetZCashBlockTracker() {
  return static_cast<ZCashBlockTracker&>(block_tracker());
}

mojom::CoinType ZCashTxManager::GetCoinType() const {
  return mojom::CoinType::ZEC;
}

void ZCashTxManager::UpdatePendingTransactions(
    const std::optional<std::string>& chain_id) {
  auto pending_transactions = tx_state_manager().GetTransactionsByStatus(
      chain_id, mojom::TransactionStatus::Submitted, std::nullopt);
  std::set<std::string> pending_chain_ids;
  for (const auto& pending_transaction : pending_transactions) {
    auto pending_chain_id = pending_transaction->chain_id();

    std::unique_ptr<ZCashTxMeta> meta =
        GetZCashTxStateManager().GetZCashTx(pending_transaction->id());
    zcash_wallet_service_->GetTransactionStatus(
        pending_transaction->from(), std::move(meta),
        base::BindOnce(&ZCashTxManager::OnGetTransactionStatus,
                       weak_factory_.GetWeakPtr(), pending_transaction->id()));
    pending_chain_ids.emplace(pending_chain_id);
  }
  CheckIfBlockTrackerShouldRun(pending_chain_ids);
}

void ZCashTxManager::OnGetTransactionStatus(
    const std::string& tx_meta_id,
    base::expected<ZCashWalletService::ResolveTransactionStatusResult,
                   std::string> confirm_status) {
  if (!confirm_status.has_value()) {
    return;
  }
  std::unique_ptr<ZCashTxMeta> meta =
      GetZCashTxStateManager().GetZCashTx(tx_meta_id);
  if (!meta) {
    return;
  }
  if (confirm_status.value() ==
      ZCashWalletService::ResolveTransactionStatusResult::kCompleted) {
    meta->set_status(mojom::TransactionStatus::Confirmed);
    meta->set_confirmed_time(base::Time::Now());
    tx_state_manager().AddOrUpdateTx(*meta);
  } else if (confirm_status.value() ==
             ZCashWalletService::ResolveTransactionStatusResult::kExpired) {
    meta->set_status(mojom::TransactionStatus::Rejected);
    tx_state_manager().AddOrUpdateTx(*meta);
  }
}

}  // namespace brave_wallet
