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

#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
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

class EthTxStateManager {
 public:
  explicit EthTxStateManager(PrefService* prefs,
                             JsonRpcService* json_rpc_service);
  ~EthTxStateManager();
  EthTxStateManager(const EthTxStateManager&) = delete;
  EthTxStateManager operator=(const EthTxStateManager&) = delete;

  static std::unique_ptr<EthTxMeta> ValueToTxMeta(const base::Value& value);

  void AddOrUpdateTx(const TxMeta& meta);
  std::unique_ptr<TxMeta> GetTx(const std::string& id);
  void DeleteTx(const std::string& id);
  void WipeTxs();

  std::vector<std::unique_ptr<TxMeta>> GetTransactionsByStatus(
      absl::optional<mojom::TransactionStatus> status,
      absl::optional<EthAddress> from);

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) {
    }
    virtual void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) {}
  };
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  std::unique_ptr<EthTxMeta> GetEthTx(const std::string& id);

 private:
  // only support REJECTED and CONFIRMED
  void RetireTxByStatus(mojom::TransactionStatus status, size_t max_num);

  std::vector<std::unique_ptr<TxMeta>> GetTransactionsByStatus(
      absl::optional<mojom::TransactionStatus> status,
      const std::string& from);

  base::ObserverList<Observer> observers_;
  raw_ptr<PrefService> prefs_ = nullptr;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;

  base::WeakPtrFactory<EthTxStateManager> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_STATE_MANAGER_H_
