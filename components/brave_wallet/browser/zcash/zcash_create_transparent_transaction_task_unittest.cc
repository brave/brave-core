// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_wallet/browser/zcash/zcash_create_transparent_transaction_task.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_action_context.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::SaveArg;

namespace brave_wallet {

constexpr char kAddress1[] = "t1WU75sSfiPbK5ez33uuhEbd9ZD3XNCxMRj";

class MockZCashRPC : public ZCashRpc {
 public:
  MockZCashRPC() : ZCashRpc(nullptr, nullptr) {}
  ~MockZCashRPC() override = default;

  MOCK_METHOD2(GetLatestBlock,
               void(const std::string& chain_id,
                    GetLatestBlockCallback callback));
};

class MockZCashWalletService : public ZCashWalletService {
 public:
  MockZCashWalletService(base::FilePath zcash_data_path,
                         KeyringService& keyring_service,
                         std::unique_ptr<ZCashRpc> zcash_rpc)
      : ZCashWalletService(zcash_data_path,
                           keyring_service,
                           std::move(zcash_rpc)) {}
  MOCK_METHOD1(CreateTransactionTaskDone,
               void(ZCashCreateTransparentTransactionTask* task));

  MOCK_METHOD3(GetUtxos,
               void(const std::string& chain_id,
                    const mojom::AccountIdPtr& account_id,
                    GetUtxosCallback));

  MOCK_METHOD3(DiscoverNextUnusedAddress,
               void(const mojom::AccountIdPtr& account_id,
                    bool change,
                    DiscoverNextUnusedAddressCallback callback));
};

class ZCashCreateTransparentTransactionTaskTest : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "false"}});
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath db_path(
        temp_dir_.GetPath().Append(FILE_PATH_LITERAL("orchard.db")));

    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    keyring_service_->Reset();
    keyring_service_->RestoreWallet(
        "gallery equal segment repair outdoor bronze limb dawn daring main "
        "burst "
        "design palm demise develop exit cycle harbor motor runway turtle "
        "quote "
        "blast tail",
        kTestWalletPassword, false, base::DoNothing());

    zcash_wallet_service_ = std::make_unique<MockZCashWalletService>(
        db_path, *keyring_service_,
        std::make_unique<testing::NiceMock<ZCashRpc>>(nullptr, nullptr));

    sync_state_ = base::SequenceBound<OrchardSyncState>(
        base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
        db_path);

    zcash_rpc_ = std::make_unique<MockZCashRPC>();

    account_id_ = AccountUtils(keyring_service_.get())
                      .EnsureAccount(mojom::KeyringId::kZCashMainnet, 0)
                      ->account_id.Clone();
  }

  MockZCashWalletService& zcash_wallet_service() {
    return *zcash_wallet_service_;
  }

  ZCashActionContext zcash_action_context() {
    return ZCashActionContext(*zcash_rpc_, {}, sync_state_, account_id_,
                              mojom::kZCashMainnet);
  }

  base::PassKey<ZCashCreateTransparentTransactionTaskTest> pass_key() {
    return base::PassKey<ZCashCreateTransparentTransactionTaskTest>();
  }

  MockZCashRPC& mock_zcash_rpc() { return *zcash_rpc_; }

  base::test::TaskEnvironment& task_environment() { return task_environment_; }

  KeyringService& keyring_service() { return *keyring_service_; }

  mojom::AccountIdPtr& account_id() { return account_id_; }

 private:
  base::test::ScopedFeatureList feature_list_;
  base::ScopedTempDir temp_dir_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  mojom::AccountIdPtr account_id_;

  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<MockZCashRPC> zcash_rpc_;
  std::unique_ptr<MockZCashWalletService> zcash_wallet_service_;

  base::SequenceBound<OrchardSyncState> sync_state_;

  base::test::TaskEnvironment task_environment_;
};

TEST_F(ZCashCreateTransparentTransactionTaskTest, TransactionCreated) {
  ON_CALL(zcash_wallet_service(), GetUtxos(_, _, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                const mojom::AccountIdPtr& account_id,
                                ZCashWalletService::GetUtxosCallback callback) {
            ZCashWalletService::UtxoMap utxo_map;
            utxo_map["60000"] = GetZCashUtxo(60000);
            utxo_map["70000"] = GetZCashUtxo(70000);
            utxo_map["80000"] = GetZCashUtxo(80000);
            std::move(callback).Run(std::move(utxo_map));
          }));

  ON_CALL(zcash_wallet_service(), DiscoverNextUnusedAddress(_, _, _))
      .WillByDefault(::testing::Invoke(
          [&](const mojom::AccountIdPtr& account_id, bool change,
              ZCashWalletService::DiscoverNextUnusedAddressCallback callback) {
            auto id = mojom::ZCashKeyId::New(account_id->account_index, 1, 0);
            auto addr = keyring_service().GetZCashAddress(account_id, *id);
            std::move(callback).Run(std::move(addr));
          }));

  ON_CALL(mock_zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(
                zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
          }));

  base::MockCallback<
      ZCashCreateTransparentTransactionTask::CreateTransactionCallback>
      callback;

  auto task = std::make_unique<ZCashCreateTransparentTransactionTask>(
      pass_key(), zcash_wallet_service(), zcash_action_context(), kAddress1,
      10000, callback.Get());

  EXPECT_CALL(zcash_wallet_service(),
              CreateTransactionTaskDone(testing::Eq(task.get())));

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start();
  task_environment().RunUntilIdle();

  EXPECT_TRUE(tx_result.has_value());

  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].amount, 10000u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].address, kAddress1);

  auto id = mojom::ZCashKeyId::New(account_id()->account_index, 1, 0);
  auto addr = keyring_service().GetZCashAddress(account_id(), *id);

  EXPECT_EQ(tx_result.value().transparent_part().outputs[1].amount, 40000u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs[1].address,
            addr->address_string);
}

TEST_F(ZCashCreateTransparentTransactionTaskTest,
       TransactionNotCreated_NoUtxos) {
  ON_CALL(zcash_wallet_service(), GetUtxos(_, _, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                const mojom::AccountIdPtr& account_id,
                                ZCashWalletService::GetUtxosCallback callback) {
            ZCashWalletService::UtxoMap utxo_map;
            std::move(callback).Run(std::move(utxo_map));
          }));

  ON_CALL(zcash_wallet_service(), DiscoverNextUnusedAddress(_, _, _))
      .WillByDefault(::testing::Invoke(
          [&](const mojom::AccountIdPtr& account_id, bool change,
              ZCashWalletService::DiscoverNextUnusedAddressCallback callback) {
            auto id = mojom::ZCashKeyId::New(account_id->account_index, 1, 0);
            auto addr = keyring_service().GetZCashAddress(account_id, *id);
            std::move(callback).Run(std::move(addr));
          }));

  ON_CALL(mock_zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(
                zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
          }));

  base::MockCallback<
      ZCashCreateTransparentTransactionTask::CreateTransactionCallback>
      callback;

  auto task = std::make_unique<ZCashCreateTransparentTransactionTask>(
      pass_key(), zcash_wallet_service(), zcash_action_context(), kAddress1,
      100000, callback.Get());

  EXPECT_CALL(zcash_wallet_service(),
              CreateTransactionTaskDone(testing::Eq(task.get())));

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start();
  task_environment().RunUntilIdle();

  EXPECT_FALSE(tx_result.has_value());
}

TEST_F(ZCashCreateTransparentTransactionTaskTest,
       TransactionNotCreated_LastBlockError) {
  ON_CALL(zcash_wallet_service(), GetUtxos(_, _, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                const mojom::AccountIdPtr& account_id,
                                ZCashWalletService::GetUtxosCallback callback) {
            ZCashWalletService::UtxoMap utxo_map;
            utxo_map["60000"] = GetZCashUtxo(60000);
            utxo_map["70000"] = GetZCashUtxo(70000);
            utxo_map["80000"] = GetZCashUtxo(80000);
            std::move(callback).Run(std::move(utxo_map));
          }));

  ON_CALL(zcash_wallet_service(), DiscoverNextUnusedAddress(_, _, _))
      .WillByDefault(::testing::Invoke(
          [&](const mojom::AccountIdPtr& account_id, bool change,
              ZCashWalletService::DiscoverNextUnusedAddressCallback callback) {
            auto id = mojom::ZCashKeyId::New(account_id->account_index, 1, 0);
            auto addr = keyring_service().GetZCashAddress(account_id, *id);
            std::move(callback).Run(std::move(addr));
          }));

  ON_CALL(mock_zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(base::unexpected("error"));
          }));

  base::MockCallback<
      ZCashCreateTransparentTransactionTask::CreateTransactionCallback>
      callback;

  auto task = std::make_unique<ZCashCreateTransparentTransactionTask>(
      pass_key(), zcash_wallet_service(), zcash_action_context(), kAddress1, 60,
      callback.Get());

  EXPECT_CALL(zcash_wallet_service(),
              CreateTransactionTaskDone(testing::Eq(task.get())));

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start();
  task_environment().RunUntilIdle();

  EXPECT_FALSE(tx_result.has_value());
}

TEST_F(ZCashCreateTransparentTransactionTaskTest,
       TransactionNotCreated_NotEnoughtFunds) {
  ON_CALL(zcash_wallet_service(), GetUtxos(_, _, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                const mojom::AccountIdPtr& account_id,
                                ZCashWalletService::GetUtxosCallback callback) {
            ZCashWalletService::UtxoMap utxo_map;
            utxo_map["60000"] = GetZCashUtxo(60000);
            std::move(callback).Run(std::move(utxo_map));
          }));

  ON_CALL(zcash_wallet_service(), DiscoverNextUnusedAddress(_, _, _))
      .WillByDefault(::testing::Invoke(
          [&](const mojom::AccountIdPtr& account_id, bool change,
              ZCashWalletService::DiscoverNextUnusedAddressCallback callback) {
            auto id = mojom::ZCashKeyId::New(account_id->account_index, 1, 0);
            auto addr = keyring_service().GetZCashAddress(account_id, *id);
            std::move(callback).Run(std::move(addr));
          }));

  ON_CALL(mock_zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(
                zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
          }));

  base::MockCallback<
      ZCashCreateTransparentTransactionTask::CreateTransactionCallback>
      callback;

  auto task = std::make_unique<ZCashCreateTransparentTransactionTask>(
      pass_key(), zcash_wallet_service(), zcash_action_context(), kAddress1,
      100000, callback.Get());

  EXPECT_CALL(zcash_wallet_service(),
              CreateTransactionTaskDone(testing::Eq(task.get())));

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start();
  task_environment().RunUntilIdle();

  EXPECT_FALSE(tx_result.has_value());
}

TEST_F(ZCashCreateTransparentTransactionTaskTest, TransactionCreated_MaxFunds) {
  ON_CALL(zcash_wallet_service(), GetUtxos(_, _, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                const mojom::AccountIdPtr& account_id,
                                ZCashWalletService::GetUtxosCallback callback) {
            ZCashWalletService::UtxoMap utxo_map;
            utxo_map["60000"] = GetZCashUtxo(60000);
            utxo_map["70000"] = GetZCashUtxo(70000);
            utxo_map["80000"] = GetZCashUtxo(80000);
            std::move(callback).Run(std::move(utxo_map));
          }));

  ON_CALL(zcash_wallet_service(), DiscoverNextUnusedAddress(_, _, _))
      .WillByDefault(::testing::Invoke(
          [&](const mojom::AccountIdPtr& account_id, bool change,
              ZCashWalletService::DiscoverNextUnusedAddressCallback callback) {
            auto id = mojom::ZCashKeyId::New(account_id->account_index, 1, 0);
            auto addr = keyring_service().GetZCashAddress(account_id, *id);
            std::move(callback).Run(std::move(addr));
          }));

  ON_CALL(mock_zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          ::testing::Invoke([](const std::string& chain_id,
                               ZCashRpc::GetLatestBlockCallback callback) {
            std::move(callback).Run(
                zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
          }));

  base::MockCallback<
      ZCashCreateTransparentTransactionTask::CreateTransactionCallback>
      callback;

  auto task = std::make_unique<ZCashCreateTransparentTransactionTask>(
      pass_key(), zcash_wallet_service(), zcash_action_context(), kAddress1,
      kZCashFullAmount, callback.Get());

  EXPECT_CALL(zcash_wallet_service(),
              CreateTransactionTaskDone(testing::Eq(task.get())));

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start();
  task_environment().RunUntilIdle();

  EXPECT_TRUE(tx_result.has_value());

  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].amount, 195000u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].address, kAddress1);
}

}  // namespace brave_wallet
