/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TX_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TX_STATE_MANAGER_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "brave/components/brave_wallet/browser/tx_state_manager.h"

class PrefService;

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class TxMeta;
class ZCashTxMeta;
class TxStorageDelegate;

class ZCashTxStateManager : public TxStateManager {
 public:
  ZCashTxStateManager(PrefService* prefs,
                      TxStorageDelegate* delegate,
                      AccountResolverDelegate* account_resolver_delegate);
  ~ZCashTxStateManager() override;
  ZCashTxStateManager(const ZCashTxStateManager&) = delete;
  ZCashTxStateManager operator=(const ZCashTxStateManager&) = delete;

  std::unique_ptr<ZCashTxMeta> GetZCashTx(const std::string& chain_id,
                                          const std::string& id);
  std::unique_ptr<ZCashTxMeta> ValueToZCashTxMeta(
      const base::Value::Dict& value);

 private:
  mojom::CoinType GetCoinType() const override;

  std::unique_ptr<TxMeta> ValueToTxMeta(
      const base::Value::Dict& value) override;
  std::string GetTxPrefPathPrefix(
      const std::optional<std::string>& chain_id) override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TX_STATE_MANAGER_H_
