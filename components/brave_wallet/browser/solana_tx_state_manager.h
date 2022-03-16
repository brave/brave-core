/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_STATE_MANAGER_H_

#include <memory>
#include <string>

#include "brave/components/brave_wallet/browser/tx_state_manager.h"

class PrefService;

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class TxMeta;
class JsonRpcService;

class SolanaTxStateManager : public TxStateManager {
 public:
  SolanaTxStateManager(PrefService* prefs, JsonRpcService* json_rpc_service);
  ~SolanaTxStateManager() override;
  SolanaTxStateManager(const SolanaTxStateManager&) = delete;
  SolanaTxStateManager operator=(const SolanaTxStateManager&) = delete;

 private:
  std::unique_ptr<TxMeta> ValueToTxMeta(const base::Value& value) override;
  std::string GetTxPrefPathPrefix() override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_STATE_MANAGER_H_
