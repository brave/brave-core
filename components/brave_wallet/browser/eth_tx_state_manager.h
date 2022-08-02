/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_STATE_MANAGER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/browser/tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class TxMeta;
class EthTxMeta;
class JsonRpcService;

class EthTxStateManager : public TxStateManager {
 public:
  explicit EthTxStateManager(PrefService* prefs,
                             JsonRpcService* json_rpc_service);
  ~EthTxStateManager() override;
  EthTxStateManager(const EthTxStateManager&) = delete;
  EthTxStateManager operator=(const EthTxStateManager&) = delete;

  std::vector<std::unique_ptr<TxMeta>> GetTransactionsByStatus(
      absl::optional<mojom::TransactionStatus> status,
      absl::optional<EthAddress> from);

  std::unique_ptr<EthTxMeta> GetEthTx(const std::string& id);
  std::unique_ptr<EthTxMeta> ValueToEthTxMeta(const base::Value::Dict& value);

 private:
  FRIEND_TEST_ALL_PREFIXES(EthTxStateManagerUnitTest, GetTxPrefPathPrefix);

  std::unique_ptr<TxMeta> ValueToTxMeta(
      const base::Value::Dict& value) override;
  std::string GetTxPrefPathPrefix() override;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_STATE_MANAGER_H_
