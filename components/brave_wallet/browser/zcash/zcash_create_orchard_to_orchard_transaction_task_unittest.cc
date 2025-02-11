/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_create_orchard_to_orchard_transaction_task.h"

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
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"

using testing::_;
using testing::SaveArg;

namespace brave_wallet {

namespace {

class MockZCashWalletService : public ZCashWalletService {
 public:
  MockZCashWalletService(base::FilePath zcash_data_path,
                         KeyringService& keyring_service,
                         std::unique_ptr<ZCashRpc> zcash_rpc)
      : ZCashWalletService(zcash_data_path,
                           keyring_service,
                           std::move(zcash_rpc)) {}
  MOCK_METHOD1(CreateTransactionTaskDone,
               void(ZCashCreateOrchardToOrchardTransactionTask* task));
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

  MOCK_METHOD1(GetSpendableNotes,
               base::expected<std::vector<OrchardNote>, OrchardStorage::Error>(
                   const mojom::AccountIdPtr& account_id));
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

class ZCashCreateOrchardToOrchardTransactionTaskTest : public testing::Test {
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

    mock_orchard_sync_state_ = std::make_unique<MockOrchardSyncState>(db_path);
    sync_state_ = base::SequenceBound<MockOrchardSyncStateProxy>(
        base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
        db_path, mock_orchard_sync_state_.get());

    zcash_rpc_ = std::make_unique<ZCashRpc>(nullptr, nullptr);

    account_id_ = AccountUtils(keyring_service_.get())
                      .EnsureAccount(mojom::KeyringId::kZCashMainnet, 0)
                      ->account_id.Clone();
  }

  void TearDown() override { sync_state_.Reset(); }

  MockOrchardSyncState& mock_orchard_sync_state() {
    return *mock_orchard_sync_state_;
  }

  MockZCashWalletService& zcash_wallet_service() {
    return *zcash_wallet_service_;
  }

  KeyringService& keyring_service() { return *keyring_service_; }

  mojom::AccountIdPtr& account_id() { return account_id_; }

  base::PassKey<class ZCashCreateOrchardToOrchardTransactionTaskTest>
  pass_key() {
    return base::PassKey<
        class ZCashCreateOrchardToOrchardTransactionTaskTest>();
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

  std::unique_ptr<MockOrchardSyncState> mock_orchard_sync_state_;
  base::SequenceBound<OrchardSyncState> sync_state_;

  base::test::TaskEnvironment task_environment_;
};

TEST_F(ZCashCreateOrchardToOrchardTransactionTaskTest, TransactionCreated) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_))
      .WillByDefault(
          ::testing::Invoke([&](const mojom::AccountIdPtr& account_id) {
            std::vector<OrchardNote> result;
            {
              OrchardNote note;
              note.block_id = 1u;
              note.amount = 70000u;
              result.push_back(std::move(note));
            }

            {
              OrchardNote note;
              note.block_id = 2u;
              note.amount = 80000u;
              result.push_back(std::move(note));
            }

            return result;
          }));

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
      "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
      "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
      "h2j",
      false);

  std::unique_ptr<ZCashCreateOrchardToOrchardTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToOrchardTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(), *orchard_part,
          std::nullopt, 100000u, callback.Get());

  EXPECT_CALL(zcash_wallet_service(),
              CreateTransactionTaskDone(testing::Eq(task.get())));

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start();

  task_environment().RunUntilIdle();

  EXPECT_TRUE(tx_result.has_value());

  EXPECT_EQ(tx_result.value().orchard_part().inputs.size(), 2u);
  EXPECT_EQ(tx_result.value().orchard_part().outputs.size(), 2u);

  EXPECT_EQ(tx_result.value().orchard_part().inputs[0].note.amount, 70000u);
  EXPECT_EQ(tx_result.value().orchard_part().inputs[1].note.amount, 80000u);

  EXPECT_EQ(tx_result.value().orchard_part().outputs[0].value, 35000u);
  EXPECT_EQ(tx_result.value().orchard_part().outputs[1].value, 100000u);
  auto change_addr = keyring_service().GetOrchardRawBytes(
      account_id(), mojom::ZCashKeyId::New(0, 1, 0));

  EXPECT_EQ(tx_result.value().orchard_part().outputs[0].addr,
            change_addr.value());
  EXPECT_EQ(tx_result.value().orchard_part().outputs[1].addr,
            orchard_part.value());
}

TEST_F(ZCashCreateOrchardToOrchardTransactionTaskTest, NotEnoughFunds) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_))
      .WillByDefault(
          ::testing::Invoke([&](const mojom::AccountIdPtr& account_id) {
            std::vector<OrchardNote> result;
            {
              OrchardNote note;
              note.block_id = 1u;
              note.amount = 70000u;
              result.push_back(std::move(note));
            }

            {
              OrchardNote note;
              note.block_id = 2u;
              note.amount = 80000u;
              result.push_back(std::move(note));
            }

            return result;
          }));

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
      "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
      "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
      "h2j",
      false);

  std::unique_ptr<ZCashCreateOrchardToOrchardTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToOrchardTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(), *orchard_part,
          std::nullopt, 1000000u, callback.Get());

  EXPECT_CALL(zcash_wallet_service(),
              CreateTransactionTaskDone(testing::Eq(task.get())));

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start();

  task_environment().RunUntilIdle();

  EXPECT_FALSE(tx_result.has_value());
}

TEST_F(ZCashCreateOrchardToOrchardTransactionTaskTest, Error) {
  ON_CALL(mock_orchard_sync_state(), GetSpendableNotes(_))
      .WillByDefault(
          ::testing::Invoke([&](const mojom::AccountIdPtr& account_id) {
            return base::unexpected(OrchardStorage::Error{
                OrchardStorage::ErrorCode::kConsistencyError, ""});
          }));

  base::MockCallback<ZCashWalletService::CreateTransactionCallback> callback;

  auto orchard_part = GetOrchardRawBytes(
      "u19hwdcqxhkapje2p0744gq96parewuffyeg0kg3q3taq040zwqh2wxjwyxzs6l9dulzua"
      "p43ya7mq7q3mu2hjafzlwylvystjlc6n294emxww9xm8qn6tcldqkq4k9ccsqzmjeqk9yp"
      "kss572ut324nmxke666jm8lhkpt85gzq58d50rfnd7wufke8jjhc3lhswxrdr57ah42xck"
      "h2j",
      false);

  std::unique_ptr<ZCashCreateOrchardToOrchardTransactionTask> task =
      std::make_unique<ZCashCreateOrchardToOrchardTransactionTask>(
          pass_key(), zcash_wallet_service(), action_context(), *orchard_part,
          std::nullopt, 1000000u, callback.Get());

  EXPECT_CALL(zcash_wallet_service(),
              CreateTransactionTaskDone(testing::Eq(task.get())));

  base::expected<ZCashTransaction, std::string> tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start();

  task_environment().RunUntilIdle();

  EXPECT_FALSE(tx_result.has_value());
}

}  // namespace brave_wallet
