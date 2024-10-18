/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_STORAGE_DELEGATE_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_STORAGE_DELEGATE_IMPL_H_

#include <memory>
#include <optional>

#include "base/gtest_prod_util.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_wallet/browser/tx_storage_delegate.h"
#include "components/value_store/value_store_factory.h"

class PrefService;

namespace value_store {
class ValueStoreFrontend;
}  // namespace value_store

namespace brave_wallet {

class TxStorageDelegateImpl final : public TxStorageDelegate {
 public:
  TxStorageDelegateImpl(
      PrefService* prefs,
      scoped_refptr<value_store::ValueStoreFactory> store_factory,
      scoped_refptr<base::SequencedTaskRunner> ui_task_runner);
  ~TxStorageDelegateImpl() override;
  TxStorageDelegateImpl(const TxStorageDelegateImpl&) = delete;
  TxStorageDelegateImpl& operator=(const TxStorageDelegateImpl&) = delete;

  bool IsInitialized() const override;
  const base::Value::Dict& GetTxs() const override;
  base::Value::Dict& GetTxs() override;
  void ScheduleWrite() override;
  void DisableWritesForTesting(bool disable);

  // Only owner ex.TxService can clear data.
  void Clear();

  void AddObserver(TxStorageDelegate::Observer* observer) override;
  void RemoveObserver(TxStorageDelegate::Observer* observer) override;

 private:
  friend class TxStateManagerUnitTest;
  friend class TxStorageDelegateImplUnitTest;
  FRIEND_TEST_ALL_PREFIXES(TxStorageDelegateImplUnitTest, ReadWriteAndClear);
  FRIEND_TEST_ALL_PREFIXES(TxStorageDelegateImplUnitTest,
                           BraveWalletTransactionsDBFormatMigrated);
  FRIEND_TEST_ALL_PREFIXES(EthTxManagerUnitTest, Reset);

  static std::unique_ptr<value_store::ValueStoreFrontend>
  MakeValueStoreFrontend(
      scoped_refptr<value_store::ValueStoreFactory> store_factory,
      scoped_refptr<base::SequencedTaskRunner> ui_task_runner);

  // Read all txs from db
  void Initialize();
  void OnTxsInitialRead(std::optional<base::Value> txs);
  void RunDBMigrations();

  bool MigrateTransactionsFromPrefsToDB();

  base::ObserverList<TxStorageDelegate::Observer> observers_;

  // Used to indicate if transactions is loaded to memory caches txs_
  bool initialized_ = false;
  // In memory txs which will be read during initialization from db and schedule
  // write to it when changed. We only hold 500 confirmed and 500 rejected
  // txs, once the limit is reached we will retire oldest entries.
  base::Value::Dict txs_;

  bool disable_writes_for_testing_ = false;

  std::unique_ptr<value_store::ValueStoreFrontend> store_;

  raw_ptr<PrefService, DanglingUntriaged> prefs_;
  base::WeakPtrFactory<TxStorageDelegateImpl> weak_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_STORAGE_DELEGATE_IMPL_H_
