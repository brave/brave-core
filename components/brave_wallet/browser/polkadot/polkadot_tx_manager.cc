/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_manager.h"

#include <utility>

#include "base/notimplemented.h"
#include "brave/components/brave_wallet/browser/account_resolver_delegate.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_block_tracker.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_meta.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

PolkadotTxManager::PolkadotTxManager(
    TxService& tx_service,
    KeyringService& keyring_service,
    TxStorageDelegate& delegate,
    AccountResolverDelegate& account_resolver_delegate)
    : TxManager(
          std::make_unique<PolkadotTxStateManager>(delegate,
                                                   account_resolver_delegate),
          std::make_unique<PolkadotBlockTracker>(),
          tx_service,
          keyring_service) {
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
    AddUnapprovedTransactionCallback callback) {
  NOTIMPLEMENTED_LOG_ONCE();

  std::move(callback).Run(false, "", "Not implemented");
}

void PolkadotTxManager::ApproveTransaction(
    const std::string& tx_meta_id,
    ApproveTransactionCallback callback) {
  NOTIMPLEMENTED_LOG_ONCE();

  std::move(callback).Run(false,
                          mojom::ProviderErrorUnion::NewProviderError(
                              mojom::ProviderError::kInternalError),
                          "Not implemented");
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
