/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_storage.h"

#include <optional>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;
using base::test::ParseJsonDict;

namespace brave_wallet {

class TxStorageUnitTest : public testing::Test {
 public:
  TxStorageUnitTest() = default;

 protected:
  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

  std::optional<base::Value> GetTxsFromDB(TxStorage* tx_storage) {
    base::RunLoop run_loop;
    std::optional<base::Value> value_out;
    tx_storage->store_->Get(
        "transactions",
        base::BindLambdaForTesting([&](std::optional<base::Value> value) {
          value_out = std::move(value);
          run_loop.Quit();
        }));
    run_loop.Run();
    return value_out;
  }

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(TxStorageUnitTest, ReadWriteAndClear) {
  auto tx_storage = CreateTxStorageForTest(temp_dir_.GetPath());
  // OnTxRead with empty txs
  auto& txs = tx_storage->txs_;
  EXPECT_TRUE(txs.empty());
  txs.Set("key1", 123);
  txs.Set("key2", base::DictValue().Set("nest", "brave"));
  tx_storage->ScheduleWrite();
  auto txs_from_db = GetTxsFromDB(tx_storage.get());
  ASSERT_TRUE(txs_from_db);
  const auto& txs_from_cache = tx_storage->GetTxs();
  EXPECT_EQ(txs_from_cache, txs_from_db);
  EXPECT_EQ(txs, txs_from_db);

  // simulate reading from existing database (with same
  // value_store::ValueStoreFrontend)
  tx_storage->initialized_ = false;
  tx_storage->txs_.clear();
  ASSERT_FALSE(tx_storage->IsInitialized());
  tx_storage->Initialize();
  WaitForTxStorageInitialized(tx_storage.get());
  ASSERT_TRUE(tx_storage->IsInitialized());
  EXPECT_EQ(tx_storage->GetTxs(), txs);

  // clear
  tx_storage->Clear();
  EXPECT_TRUE(tx_storage->IsInitialized());
  EXPECT_TRUE(tx_storage->GetTxs().empty());
  EXPECT_FALSE(GetTxsFromDB(tx_storage.get()));
}

TEST_F(TxStorageUnitTest, ReadWriteAndClearInMemory) {
  auto tx_storage = TxStorage::MakeWithMemoryOnlyStorage();
  ASSERT_EQ(tx_storage->store_, nullptr);
  ASSERT_FALSE(tx_storage->IsInitialized());

  WaitForTxStorageInitialized(tx_storage.get());
  ASSERT_TRUE(tx_storage->IsInitialized());

  // Start with empty txs
  auto& txs = tx_storage->txs_;
  EXPECT_TRUE(txs.empty());
  txs.Set("key1", 123);
  txs.Set("key2", base::DictValue().Set("nest", "brave"));

  const auto& txs_from_cache = tx_storage->GetTxs();
  EXPECT_EQ(txs, txs_from_cache);

  // clear
  tx_storage->Clear();
  EXPECT_TRUE(tx_storage->IsInitialized());
  EXPECT_TRUE(tx_storage->GetTxs().empty());
}

}  // namespace brave_wallet
