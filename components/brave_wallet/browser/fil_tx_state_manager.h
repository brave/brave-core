/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_TX_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_TX_STATE_MANAGER_H_

#include <memory>
#include <string>

#include <utility>
#include <vector>
#include "brave/components/brave_wallet/browser/tx_state_manager.h"

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/browser/tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class TxMeta;
class FilTxMeta;
class JsonRpcService;

class FilTxStateManager : public TxStateManager {
 public:
  FilTxStateManager(PrefService* prefs, JsonRpcService* json_rpc_service);
  ~FilTxStateManager() override;
  FilTxStateManager(const FilTxStateManager&) = delete;
  FilTxStateManager operator=(const FilTxStateManager&) = delete;

  std::unique_ptr<FilTxMeta> GetFilTx(const std::string& id);

 private:
  std::unique_ptr<TxMeta> ValueToTxMeta(const base::Value& value) override;
  std::string GetTxPrefPathPrefix() override;

  std::string chain_id_;
  std::string network_url_;
  base::WeakPtrFactory<FilTxStateManager> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_TX_STATE_MANAGER_H_
