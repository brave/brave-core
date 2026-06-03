/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_STORAGE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_STORAGE_H_

#include <memory>
#include <optional>

#include "base/functional/callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "components/value_store/value_store_factory.h"
#include "components/value_store/value_store_frontend.h"

namespace brave_wallet {

class TxStorage {
 public:
  explicit TxStorage(std::unique_ptr<value_store::ValueStoreFrontend> store);
  ~TxStorage();
  TxStorage(const TxStorage&) = delete;
  TxStorage& operator=(const TxStorage&) = delete;

  class ScopedTxsUpdate {
   public:
    explicit ScopedTxsUpdate(TxStorage& tx_storage);
    ScopedTxsUpdate(const ScopedTxsUpdate&) = delete;
    ScopedTxsUpdate& operator=(const ScopedTxsUpdate&) = delete;
    ~ScopedTxsUpdate();

    base::DictValue& Get();

    base::DictValue& operator*() { return Get(); }
    base::DictValue* operator->() { return &Get(); }

   private:
    const raw_ref<TxStorage> tx_storage_;
  };

  bool IsInitialized() const;
  const base::DictValue& GetTxs() const;
  void Clear();

  ScopedTxsUpdate CreateScopedTxsUpdate();

  void SetOnInitializedCallbackForTesting(base::OnceClosure callback);
  void DisableWritesForTesting(bool disable);

  static std::unique_ptr<TxStorage> MakeWithDbStorage(
      const base::FilePath& wallet_base_directory);
  static std::unique_ptr<TxStorage> MakeWithMemoryOnlyStorage();

 private:
  friend class TxStateManagerUnitTest;
  friend class TxStorageUnitTest;
  FRIEND_TEST_ALL_PREFIXES(TxStorageUnitTest, ReadWriteAndClear);
  FRIEND_TEST_ALL_PREFIXES(TxStorageUnitTest, ReadWriteAndClearInMemory);
  FRIEND_TEST_ALL_PREFIXES(TxStorageUnitTest,
                           BraveWalletTransactionsDBFormatMigrated);
  FRIEND_TEST_ALL_PREFIXES(EthTxManagerUnitTest, Reset);

  // Read all txs from db
  void Initialize();
  void OnTxsInitialRead(std::optional<base::Value> txs);
  void RunDBMigrations();
  void ScheduleWrite();

  // Used to indicate if transactions is loaded to memory caches txs_
  bool initialized_ = false;
  // In memory txs which will be read during initialization from db and schedule
  // write to it when changed. We only hold 500 confirmed and 500 rejected
  // txs, once the limit is reached we will retire oldest entries.
  base::DictValue txs_;

  bool disable_writes_for_testing_ = false;
  base::OnceClosure on_initialized_callback_for_testing_;

  std::unique_ptr<value_store::ValueStoreFrontend> store_;

  base::WeakPtrFactory<TxStorage> weak_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_STORAGE_H_
