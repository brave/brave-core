/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_STATE_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_STATE_MANAGER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/observer_list_types.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class TxMeta;
class JsonRpcService;

class TxStateManager {
 public:
  TxStateManager(PrefService* prefs, JsonRpcService* json_rpc_service);
  virtual ~TxStateManager();
  TxStateManager(const TxStateManager&) = delete;

  void AddOrUpdateTx(const TxMeta& meta);
  std::unique_ptr<TxMeta> GetTx(const std::string& id);
  void DeleteTx(const std::string& id);
  void WipeTxs();

  std::vector<std::unique_ptr<TxMeta>> GetTransactionsByStatus(
      absl::optional<mojom::TransactionStatus> status,
      absl::optional<std::string> from);

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) {
    }
    virtual void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) {}
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  // For derived classes to call to fill TxMeta properties.
  static bool ValueToTxMeta(const base::Value::Dict& value, TxMeta* tx_meta);

  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;

 private:
  FRIEND_TEST_ALL_PREFIXES(TxStateManagerUnitTest, TxOperations);
  void RetireTxByStatus(mojom::TransactionStatus status, size_t max_num);

  // Each derived class should implement its own ValueToTxMeta to create a
  // specific type of tx meta (ex: EthTxMeta) from a value. TxMeta
  // properties can be filled via the protected ValueToTxMeta function above.
  virtual std::unique_ptr<TxMeta> ValueToTxMeta(
      const base::Value::Dict& value) = 0;

  // Each derived class should provide transaction pref path prefix as
  // coin_type.network_id. For example, ethereum.mainnet or solana.testnet.
  // This will be used to get/set the transaction pref for a specific
  // coin_type.
  virtual std::string GetTxPrefPathPrefix() = 0;

  base::ObserverList<Observer> observers_;

  base::WeakPtrFactory<TxStateManager> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_STATE_MANAGER_H_
