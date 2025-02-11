/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_create_transparent_to_orchard_transaction_task.h"

#include <memory>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/threading/sequence_bound.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"

using testing::_;
using testing::SaveArg;

namespace brave_wallet {

namespace {

constexpr char kReceiverAddr[] =
    "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
    "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
    "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
    "h2j";

class MockZCashWalletService : public ZCashWalletService {
 public:
  MockZCashWalletService(base::FilePath zcash_data_path,
                         KeyringService& keyring_service,
                         std::unique_ptr<ZCashRpc> zcash_rpc)
      : ZCashWalletService(zcash_data_path,
                           keyring_service,
                           std::move(zcash_rpc)) {}
  MOCK_METHOD1(CreateTransactionTaskDone,
               void(ZCashCreateTransparentToOrchardTransactionTask* task));

  MOCK_METHOD3(GetUtxos,
               void(const std::string& chain_id,
                    const mojom::AccountIdPtr& account_id,
                    GetUtxosCallback));

  MOCK_METHOD3(DiscoverNextUnusedAddress,
               void(const mojom::AccountIdPtr& account_id,
                    bool change,
                    DiscoverNextUnusedAddressCallback callback));
};

class MockZCashRPC : public ZCashRpc {
 public:
  MockZCashRPC() : ZCashRpc(nullptr, nullptr) {}
  ~MockZCashRPC() override {}
};

class MockOrchardSyncState : public OrchardSyncState {
 public:
  explicit MockOrchardSyncState(const base::FilePath& path_to_database)
      : OrchardSyncState(path_to_database) {}
  ~MockOrchardSyncState() override {}
};

class MockOrchardSyncStateProxy : public OrchardSyncState {
 public:
  MockOrchardSyncStateProxy(const base::FilePath& file_path,
                            OrchardSyncState* instance)
      : OrchardSyncState(file_path), instance_(instance) {}

  ~MockOrchardSyncStateProxy() override {}

  base::expected<std::vector<OrchardNote>, OrchardStorage::Error>
  GetSpendableNotes(const mojom::AccountIdPtr& account_id) override {
    return instance_->GetSpendableNotes(account_id);
  }

 private:
  raw_ptr<OrchardSyncState> instance_;
};

}  // namespace

class ZCashCreateTransparentToOrchardTransactionTaskTest
    : public testing::Test {
 public:
  void SetUp() override {
    feature_list_.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"}});
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

    zcash_rpc_ = std::make_unique<ZCashRpc>(nullptr, nullptr);

    account_id_ = AccountUtils(keyring_service_.get())
                      .EnsureAccount(mojom::KeyringId::kZCashMainnet, 0)
                      ->account_id.Clone();
  }

  void TearDown() override { sync_state_.Reset(); }

  MockZCashWalletService& zcash_wallet_service() {
    return *zcash_wallet_service_;
  }

  KeyringService& keyring_service() { return *keyring_service_; }

  mojom::AccountIdPtr& account_id() { return account_id_; }

  base::PassKey<class ZCashCreateTransparentToOrchardTransactionTaskTest>
  pass_key() {
    return base::PassKey<
        class ZCashCreateTransparentToOrchardTransactionTaskTest>();
  }

  ZCashActionContext action_context() {
    return ZCashActionContext(*zcash_rpc_, sync_state_, account_id_,
                              mojom::kZCashMainnet);
  }

  base::test::TaskEnvironment& task_environment() { return task_environment_; }

 private:
  base::test::ScopedFeatureList feature_list_;
  base::ScopedTempDir temp_dir_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  mojom::AccountIdPtr account_id_;

  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<ZCashRpc> zcash_rpc_;
  std::unique_ptr<MockZCashWalletService> zcash_wallet_service_;

  base::SequenceBound<OrchardSyncState> sync_state_;

  base::test::TaskEnvironment task_environment_;
};

TEST_F(ZCashCreateTransparentToOrchardTransactionTaskTest, TransactionCreated) {
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
            auto addr = keyring_service().GetZCashAddress(*account_id, *id);
            std::move(callback).Run(std::move(addr));
          }));

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(kReceiverAddr, false);

  std::unique_ptr<ZCashCreateTransparentToOrchardTransactionTask> task =
      std::make_unique<ZCashCreateTransparentToOrchardTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(), *orchard_part,
          std::nullopt, 100000u, callback.Get());

  EXPECT_CALL(zcash_wallet_service(),
              CreateTransactionTaskDone(testing::Eq(task.get())));

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start();

  task_environment().RunUntilIdle();

  EXPECT_TRUE(tx_result.has_value());

  EXPECT_EQ(tx_result.value().transparent_part().inputs.size(), 2u);
  EXPECT_EQ(tx_result.value().transparent_part().outputs.size(), 1u);

  EXPECT_EQ(tx_result.value().orchard_part().inputs.size(), 0u);
  EXPECT_EQ(tx_result.value().orchard_part().outputs.size(), 1u);

  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].amount, 10000u);
  EXPECT_EQ(tx_result.value().orchard_part().outputs[0].value, 100000u);

  auto id = mojom::ZCashKeyId::New(account_id()->account_index, 1, 0);
  auto addr = keyring_service().GetZCashAddress(*account_id(), *id);

  EXPECT_EQ(tx_result.value().transparent_part().outputs[0].address,
            addr->address_string);
  EXPECT_EQ(tx_result.value().orchard_part().outputs[0].addr,
            orchard_part.value());
}

TEST_F(ZCashCreateTransparentToOrchardTransactionTaskTest, NotEnoughFunds) {
  ON_CALL(zcash_wallet_service(), GetUtxos(_, _, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                const mojom::AccountIdPtr& account_id,
                                ZCashWalletService::GetUtxosCallback callback) {
            ZCashWalletService::UtxoMap utxo_map;
            utxo_map["6000"] = GetZCashUtxo(6000);
            utxo_map["7000"] = GetZCashUtxo(7000);
            utxo_map["8000"] = GetZCashUtxo(8000);
            std::move(callback).Run(std::move(utxo_map));
          }));

  ON_CALL(zcash_wallet_service(), DiscoverNextUnusedAddress(_, _, _))
      .WillByDefault(::testing::Invoke(
          [&](const mojom::AccountIdPtr& account_id, bool change,
              ZCashWalletService::DiscoverNextUnusedAddressCallback callback) {
            auto id = mojom::ZCashKeyId::New(account_id->account_index, 1, 0);
            auto addr = keyring_service().GetZCashAddress(*account_id, *id);
            std::move(callback).Run(std::move(addr));
          }));

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(kReceiverAddr, false);

  std::unique_ptr<ZCashCreateTransparentToOrchardTransactionTask> task =
      std::make_unique<ZCashCreateTransparentToOrchardTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(), *orchard_part,
          std::nullopt, 100000u, callback.Get());

  EXPECT_CALL(zcash_wallet_service(),
              CreateTransactionTaskDone(testing::Eq(task.get())));

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start();

  task_environment().RunUntilIdle();

  EXPECT_FALSE(tx_result.has_value());
}

TEST_F(ZCashCreateTransparentToOrchardTransactionTaskTest, UtxosError) {
  ON_CALL(zcash_wallet_service(), GetUtxos(_, _, _))
      .WillByDefault(
          ::testing::Invoke([&](const std::string& chain_id,
                                const mojom::AccountIdPtr& account_id,
                                ZCashWalletService::GetUtxosCallback callback) {
            std::move(callback).Run(base::unexpected("error"));
          }));

  ON_CALL(zcash_wallet_service(), DiscoverNextUnusedAddress(_, _, _))
      .WillByDefault(::testing::Invoke(
          [&](const mojom::AccountIdPtr& account_id, bool change,
              ZCashWalletService::DiscoverNextUnusedAddressCallback callback) {
            auto id = mojom::ZCashKeyId::New(account_id->account_index, 1, 0);
            auto addr = keyring_service().GetZCashAddress(*account_id, *id);
            std::move(callback).Run(std::move(addr));
          }));

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(kReceiverAddr, false);

  std::unique_ptr<ZCashCreateTransparentToOrchardTransactionTask> task =
      std::make_unique<ZCashCreateTransparentToOrchardTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(), *orchard_part,
          std::nullopt, 100000u, callback.Get());

  EXPECT_CALL(zcash_wallet_service(),
              CreateTransactionTaskDone(testing::Eq(task.get())));

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start();

  task_environment().RunUntilIdle();

  EXPECT_FALSE(tx_result.has_value());
}

TEST_F(ZCashCreateTransparentToOrchardTransactionTaskTest, ChangeAddressError) {
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
            std::move(callback).Run(base::unexpected("error"));
          }));

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(kReceiverAddr, false);

  std::unique_ptr<ZCashCreateTransparentToOrchardTransactionTask> task =
      std::make_unique<ZCashCreateTransparentToOrchardTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(), *orchard_part,
          std::nullopt, 100000u, callback.Get());

  EXPECT_CALL(zcash_wallet_service(),
              CreateTransactionTaskDone(testing::Eq(task.get())));

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start();

  task_environment().RunUntilIdle();

  EXPECT_FALSE(tx_result.has_value());
}

}  // namespace brave_wallet
