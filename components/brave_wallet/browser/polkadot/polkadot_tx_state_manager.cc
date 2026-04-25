/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_state_manager.h"

#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

PolkadotTxStateManager::PolkadotTxStateManager(
    TxStorageDelegate& delegate,
    AccountResolverDelegate& account_resolver_delegate)
    : TxStateManager(delegate, account_resolver_delegate) {}

PolkadotTxStateManager::~PolkadotTxStateManager() = default;

mojom::CoinType PolkadotTxStateManager::GetCoinType() const {
  return mojom::CoinType::DOT;
}

std::unique_ptr<TxMeta> PolkadotTxStateManager::ValueToTxMeta(
    const base::DictValue& value) {
  auto tx_meta = std::make_unique<PolkadotTxMeta>();

  if (!ValueToBaseTxMeta(value, tx_meta.get())) {
    return nullptr;
  }

  const auto* tx_json = value.FindDict("tx");
  if (!tx_json) {
    return nullptr;
  }

  auto tx = PolkadotTransaction::FromValue(*tx_json);
  if (!tx) {
    return nullptr;
  }

  tx_meta->set_tx(std::move(*tx));

  return tx_meta;
}

}  // namespace brave_wallet
