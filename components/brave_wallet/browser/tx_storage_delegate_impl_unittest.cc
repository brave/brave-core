/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_storage_delegate_impl.h"

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
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;
using base::test::ParseJsonDict;

namespace brave_wallet {

class TxStorageDelegateImplUnitTest : public testing::Test {
 public:
  TxStorageDelegateImplUnitTest() {}

 protected:
  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    factory_ = GetTestValueStoreFactory(temp_dir_);
  }

  std::optional<base::Value> GetTxsFromDB(TxStorageDelegateImpl* delegate) {
    base::RunLoop run_loop;
    std::optional<base::Value> value_out;
    delegate->store_->Get(
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
  scoped_refptr<value_store::TestValueStoreFactory> factory_;
};

TEST_F(TxStorageDelegateImplUnitTest, ReadWriteAndClear) {
  auto delegate = GetTxStorageDelegateForTest(&prefs_, factory_);
  // OnTxRead with empty txs
  auto& txs = delegate->GetTxs();
  EXPECT_TRUE(txs.empty());
  txs.Set("key1", 123);
  txs.Set("key2", base::DictValue().Set("nest", "brave"));
  delegate->ScheduleWrite();
  auto txs_from_db = GetTxsFromDB(delegate.get());
  ASSERT_TRUE(txs_from_db);
  const auto& txs_from_cache = delegate->GetTxs();
  EXPECT_EQ(txs_from_cache, txs_from_db);
  EXPECT_EQ(txs, txs_from_db);

  // simulate reading from existing database (with same
  // value_store::ValueStoreFrontend)
  delegate->initialized_ = false;
  delegate->txs_.clear();
  ASSERT_FALSE(delegate->IsInitialized());
  delegate->Initialize();
  WaitForTxStorageDelegateInitialized(delegate.get());
  ASSERT_TRUE(delegate->IsInitialized());
  EXPECT_EQ(delegate->GetTxs(), txs);

  // clear
  delegate->Clear();
  EXPECT_TRUE(delegate->IsInitialized());
  EXPECT_TRUE(delegate->GetTxs().empty());
  EXPECT_FALSE(GetTxsFromDB(delegate.get()));
}

}  // namespace brave_wallet
