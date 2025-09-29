/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TX_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TX_STATE_MANAGER_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace brave_wallet {

class TxStorageDelegate;
class AccountResolverDelegate;

class PolkadotTxStateManager : public TxStateManager {
 public:
  explicit PolkadotTxStateManager(
      TxStorageDelegate& delegate,
      AccountResolverDelegate& account_resolver_delegate);
  ~PolkadotTxStateManager() override;

  PolkadotTxStateManager(const PolkadotTxStateManager&) = delete;
  PolkadotTxStateManager& operator=(const PolkadotTxStateManager&) = delete;

 protected:
  // TxStateManager
  mojom::CoinType GetCoinType() const override;
  std::unique_ptr<TxMeta> ValueToTxMeta(
      const base::Value::Dict& value) override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_POLKADOT_POLKADOT_TX_STATE_MANAGER_H_
