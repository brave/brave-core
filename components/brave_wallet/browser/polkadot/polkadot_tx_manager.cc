/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_manager.h"

#include <utility>

#include "base/notimplemented.h"
#include "brave/components/brave_wallet/browser/account_resolver_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_block_tracker.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_meta.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_utils.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_wallet_service.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

PolkadotTxManager::PolkadotTxManager(
    TxService& tx_service,
    PolkadotWalletService& polkadot_wallet_service,
    KeyringService& keyring_service,
    TxStorageDelegate& delegate,
    AccountResolverDelegate& account_resolver_delegate)
    : TxManager(
          std::make_unique<PolkadotTxStateManager>(delegate,
                                                   account_resolver_delegate),
          std::make_unique<PolkadotBlockTracker>(),
          tx_service,
          keyring_service),
      polkadot_wallet_service_(polkadot_wallet_service) {
  GetPolkadotBlockTracker().AddObserver(this);
}

PolkadotTxManager::~PolkadotTxManager() {
  GetPolkadotBlockTracker().RemoveObserver(this);
}

void PolkadotTxManager::AddUnapprovedTransaction(
    const std::string& chain_id,
    mojom::TxDataUnionPtr tx_data_union,
    const mojom::AccountIdPtr& from,
    const std::optional<url::Origin>& origin,
    mojom::SwapInfoPtr swap_info,
    AddUnapprovedTransactionCallback callback) {
  NOTIMPLEMENTED_LOG_ONCE();

  std::move(callback).Run(false, "", "Not implemented");
}

std::unique_ptr<PolkadotTxMeta> PolkadotTxManager::GetPolkadotTx(
    const std::string& tx_meta_id) {
  auto tx_meta = tx_state_manager().GetTx(tx_meta_id);
  if (!tx_meta) {
    return {};
  }

  // Because our tx_state_manager() points to the base object of the
  // PolkadotTxStateManager, we dispatch correctly to the
  // PolkadotTxStateManager::ValueToTxMeta() definition which creates a complete
  // PolkadotTxMeta structure which makes this cast well-defined.
  return base::WrapUnique<PolkadotTxMeta>(
      static_cast<PolkadotTxMeta*>(tx_meta.release()));
}

void PolkadotTxManager::ApproveTransaction(
    const std::string& tx_meta_id,
    ApproveTransactionCallback callback) {
  auto tx_meta = GetPolkadotTx(tx_meta_id);
  if (!tx_meta) {
    std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewPolkadotProviderError(
            mojom::PolkadotProviderError::kInternalError),
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_TRANSACTION_NOT_FOUND));
    return;
  }

  const auto& chain_id = tx_meta->chain_id();
  auto send_amount = tx_meta->tx()->amount();
  auto recipient = tx_meta->tx()->recipient().pubkey;

  // tx_meta->set_status(mojom::TransactionStatus::Approved);
  // if (!tx_state_manager().AddOrUpdateTx(*tx_meta)) {
  //   std::move(callback).Run(false,
  //                           mojom::ProviderErrorUnion::NewPolkadotProviderError(
  //                               mojom::PolkadotProviderError::kInternalError),
  //                           WalletInternalErrorMessage());
  //   return;
  // }

  const auto& account_id = tx_meta->from();

  polkadot_wallet_service_->SignAndSendTransaction(
      chain_id, account_id, send_amount, recipient,
      base::BindOnce(&PolkadotTxManager::OnApprovePolkadotTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(tx_meta),
                     std::move(callback)));
}

void PolkadotTxManager::OnApprovePolkadotTransaction(
    std::unique_ptr<PolkadotTxMeta> tx_meta,
    ApproveTransactionCallback callback,
    base::expected<std::string, std::string> tx_hash) {
  CHECK(tx_meta);
  if (!tx_hash.has_value()) {
    tx_meta->set_status(mojom::TransactionStatus::Error);
  } else {
    tx_meta->set_status(mojom::TransactionStatus::Submitted);
    tx_meta->set_submitted_time(base::Time::Now());
    tx_meta->set_tx_hash(tx_hash.value());
  }

  if (!tx_state_manager().AddOrUpdateTx(*tx_meta)) {
    return std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewPolkadotProviderError(
            mojom::PolkadotProviderError::kInternalError),
        WalletInternalErrorMessage());
  }

  if (!tx_hash.has_value()) {
    return std::move(callback).Run(
        false,
        mojom::ProviderErrorUnion::NewPolkadotProviderError(
            mojom::PolkadotProviderError::kInternalError),
        tx_hash.error());
  }

  return std::move(callback).Run(
      true,
      mojom::ProviderErrorUnion::NewPolkadotProviderError(
          mojom::PolkadotProviderError::kSuccess),
      tx_hash.value());
}

void PolkadotTxManager::AddUnapprovedPolkadotTransaction(
    mojom::NewPolkadotTransactionParamsPtr params,
    AddUnapprovedPolkadotTransactionCallback callback) {
  auto chain_id = params->chain_id;
  if (chain_id != GetNetworkForPolkadotAccount(params->from)) {
    return std::move(callback).Run(false, "", WalletInternalErrorMessage());
  }

  polkadot_wallet_service_->GetChainMetadata(
      chain_id,
      base::BindOnce(&PolkadotTxManager::OnGetChainMetadataForUnapproved,
                     weak_ptr_factory_.GetWeakPtr(), std::move(params),
                     std::move(callback)));
}

void PolkadotTxManager::OnGetChainMetadataForUnapproved(
    mojom::NewPolkadotTransactionParamsPtr params,
    AddUnapprovedPolkadotTransactionCallback callback,
    const base::expected<PolkadotChainMetadata, std::string>& chain_metadata) {
  if (!chain_metadata.has_value()) {
    return std::move(callback).Run(false, "", chain_metadata.error());
  }

  auto recipient =
      ParsePolkadotAccount(params->to, chain_metadata->GetSs58Prefix());
  if (!recipient) {
    return std::move(callback).Run(false, "", WalletInternalErrorMessage());
  }

  // We don't support Polkadot dApps so far, so all transactions come from
  // wallet origin.
  std::optional<url::Origin> origin = std::nullopt;

  PolkadotTxMeta tx_metadata;

  PolkadotTransaction tx;
  tx.set_amount(MojomToUint128(params->amount));
  tx.set_recipient(*recipient);
  tx.set_transfer_all(params->sending_max_amount);
  tx_metadata.set_tx(std::move(tx));

  tx_metadata.set_from(params->from);
  tx_metadata.set_id(TxMeta::GenerateMetaID());
  tx_metadata.set_origin(
      origin.value_or(url::Origin::Create(GURL("chrome://wallet"))));
  tx_metadata.set_created_time(base::Time::Now());
  tx_metadata.set_status(mojom::TransactionStatus::Unapproved);
  tx_metadata.set_chain_id(params->chain_id);
  tx_metadata.set_swap_info(std::move(params->swap_info));

  if (!tx_state_manager().AddOrUpdateTx(tx_metadata)) {
    std::move(callback).Run(false, "", WalletInternalErrorMessage());
    return;
  }

  std::move(callback).Run(true, tx_metadata.id(), "");
}

void PolkadotTxManager::SpeedupOrCancelTransaction(
    const std::string& tx_meta_id,
    bool cancel,
    SpeedupOrCancelTransactionCallback callback) {
  NOTIMPLEMENTED_LOG_ONCE();

  std::move(callback).Run(false, "", "Not implemented");
}

void PolkadotTxManager::RetryTransaction(const std::string& tx_meta_id,
                                         RetryTransactionCallback callback) {
  NOTIMPLEMENTED_LOG_ONCE();

  std::move(callback).Run(false, "", "Not implemented");
}

void PolkadotTxManager::Reset() {
  NOTIMPLEMENTED_LOG_ONCE();
}

mojom::CoinType PolkadotTxManager::GetCoinType() const {
  return mojom::CoinType::DOT;
}

void PolkadotTxManager::UpdatePendingTransactions(
    const std::optional<std::string>& chain_id) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void PolkadotTxManager::OnLatestBlock(const std::string& chain_id,
                                      uint64_t block_num) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void PolkadotTxManager::OnNewBlock(const std::string& chain_id,
                                   uint64_t block_num) {
  NOTIMPLEMENTED_LOG_ONCE();
}

PolkadotBlockTracker& PolkadotTxManager::GetPolkadotBlockTracker() {
  return static_cast<PolkadotBlockTracker&>(block_tracker());
}

}  // namespace brave_wallet
