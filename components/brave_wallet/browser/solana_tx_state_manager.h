/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_STATE_MANAGER_H_

#include <memory>
#include <string>

#include "brave/components/brave_wallet/browser/tx_state_manager.h"

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class TxMeta;
class TxStorageDelegate;
class SolanaTxMeta;

class SolanaTxStateManager : public TxStateManager {
 public:
  SolanaTxStateManager(TxStorageDelegate& delegate,
                       AccountResolverDelegate& account_resolver_delegate);
  ~SolanaTxStateManager() override;
  SolanaTxStateManager(const SolanaTxStateManager&) = delete;
  SolanaTxStateManager operator=(const SolanaTxStateManager&) = delete;

  std::unique_ptr<SolanaTxMeta> GetSolanaTx(const std::string& id);
  std::unique_ptr<SolanaTxMeta> ValueToSolanaTxMeta(
      const base::Value::Dict& value);

 private:
  mojom::CoinType GetCoinType() const override;

  std::unique_ptr<TxMeta> ValueToTxMeta(
      const base::Value::Dict& value) override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_STATE_MANAGER_H_
