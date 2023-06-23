/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

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

namespace value_store {
class ValueStoreFrontend;
}  // namespace value_store

namespace brave_wallet {

class TxMeta;

class TxStateManager {
 public:
  TxStateManager(PrefService* prefs, value_store::ValueStoreFrontend* store);
  virtual ~TxStateManager();
  TxStateManager(const TxStateManager&) = delete;

  bool AddOrUpdateTx(const TxMeta& meta);
  std::unique_ptr<TxMeta> GetTx(const std::string& chain_id,
                                const std::string& id);
  bool DeleteTx(const std::string& chain_id, const std::string& id);
  bool WipeTxs();

  static void MigrateAddChainIdToTransactionInfo(PrefService* prefs);
  static void MigrateSolanaTransactionsForV0TransactionsSupport(
      PrefService* prefs);
  static void MigrateTransactionsFromPrefsToDB(
      PrefService* prefs,
      value_store::ValueStoreFrontend* store);

  std::vector<std::unique_ptr<TxMeta>> GetTransactionsByStatus(
      const absl::optional<std::string>& chain_id,
      const absl::optional<mojom::TransactionStatus>& status,
      const absl::optional<std::string>& from);

  class Observer : public base::CheckedObserver {
   public:
    virtual void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx_info) {
    }
    virtual void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx_info) {}
    virtual void OnInitialized() {}
  };

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 protected:
  // For derived classes to call to fill TxMeta properties.
  static bool ValueToTxMeta(const base::Value::Dict& value, TxMeta* tx_meta);

  raw_ptr<PrefService> prefs_ = nullptr;

 private:
  FRIEND_TEST_ALL_PREFIXES(TxStateManagerUnitTest, TxOperations);
  FRIEND_TEST_ALL_PREFIXES(TxStateManagerUnitTest,
                           MigrateTransactionsFromPrefsToDB);
  FRIEND_TEST_ALL_PREFIXES(EthTxManagerUnitTest, Reset);

  // Read all txs from db
  void Initialize();
  void OnTxsRead(absl::optional<base::Value> txs);

  // Post task to valure store to write to db.
  void ScheduleWrite();

  void RetireTxByStatus(const std::string& chain_id,
                        mojom::TransactionStatus status,
                        size_t max_num);

  virtual mojom::CoinType GetCoinType() const = 0;

  // Each derived class should implement its own ValueToTxMeta to create a
  // specific type of tx meta (ex: EthTxMeta) from a value. TxMeta
  // properties can be filled via the protected ValueToTxMeta function above.
  virtual std::unique_ptr<TxMeta> ValueToTxMeta(
      const base::Value::Dict& value) = 0;

  // Each derived class should provide transaction pref path prefix as
  // coin_type.network_id. For example, ethereum.mainnet or solana.testnet.
  // This will be used to get/set the transaction pref for a specific
  // coin_type. When chain_id is not provided, prefix will be just coin_type,
  // ex. ethereum and solana and it will be used to acess all the transactions
  // across different network for the coin.
  virtual std::string GetTxPrefPathPrefix(
      const absl::optional<std::string>& chain_id) = 0;

  bool initialized_;
  // In memory txs which will be read during initialization from db and schedule
  // write to it when changed. We only hold 500 confirmed and 500 rejected
  // txs, once the limit is reached we will retire oldest entries.
  base::Value::Dict txs_;
  base::ObserverList<Observer> observers_;
  raw_ptr<value_store::ValueStoreFrontend> store_;
  base::WeakPtrFactory<TxStateManager> weak_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_STATE_MANAGER_H_
