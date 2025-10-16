/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/task/thread_pool.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_storage.h"
#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_action_context.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_get_zcash_chain_tip_status_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Eq;
using testing::SaveArg;
using testing::Truly;
using testing::WithArg;

namespace brave_wallet {

namespace {

class MockZCashRPC : public ZCashRpc {
 public:
  MockZCashRPC() : ZCashRpc(nullptr, nullptr) {}
  ~MockZCashRPC() override = default;

  MOCK_METHOD2(GetLatestBlock,
               void(const std::string& chain_id,
                    GetLatestBlockCallback callback));
};

class MockOrchardSyncState : public OrchardSyncState {
 public:
  explicit MockOrchardSyncState(const base::FilePath& path_to_database)
      : OrchardSyncState(path_to_database) {}
  ~MockOrchardSyncState() override {}

  MOCK_METHOD1(GetAccountMeta,
               base::expected<std::optional<OrchardStorage::AccountMeta>,
                              OrchardStorage::Error>(
                   const mojom::AccountIdPtr& account_id));
};

class MockOrchardSyncStateProxy : public OrchardSyncState {
 public:
  MockOrchardSyncStateProxy(const base::FilePath& file_path,
                            OrchardSyncState* instance)
      : OrchardSyncState(file_path), instance_(instance) {}

  ~MockOrchardSyncStateProxy() override {}

  base::expected<std::optional<OrchardStorage::AccountMeta>,
                 OrchardStorage::Error>
  GetAccountMeta(const mojom::AccountIdPtr& account_id) override {
    return instance_->GetAccountMeta(account_id);
  }

 private:
  raw_ptr<OrchardSyncState> instance_;
};

}  // namespace

class ZCashGetChainTipStatusTaskTest : public testing::Test {
 public:
  ZCashGetChainTipStatusTaskTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    feature_list_.InitAndEnableFeatureWithParameters(
        features::kBraveWalletZCashFeature,
        {{"zcash_shielded_transactions_enabled", "true"}});
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath db_path(
        temp_dir_.GetPath().Append(FILE_PATH_LITERAL("orchard.db")));
    account_id_ = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                          mojom::KeyringId::kZCashMainnet,
                                          mojom::AccountKind::kDerived, 0);

    mocked_sync_state_ = std::make_unique<MockOrchardSyncState>(db_path);
    sync_state_ = base::SequenceBound<MockOrchardSyncStateProxy>(
        base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()}),
        db_path, mocked_sync_state_.get());

    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    keyring_service_->Reset();
    keyring_service_->RestoreWallet(kMnemonicGalleryEqual, kTestWalletPassword,
                                    false, base::DoNothing());

    zcash_wallet_service_ = std::make_unique<ZCashWalletService>(
        db_path, *keyring_service_,
        std::make_unique<testing::NiceMock<ZCashRpc>>(nullptr, nullptr));
  }

  void TearDown() override {
    sync_state_.Reset();
    task_environment().RunUntilIdle();
  }

  ZCashActionContext CreateContext() {
    return ZCashActionContext(zcash_rpc_, {}, sync_state_, account_id_);
  }

  testing::NiceMock<MockZCashRPC>& zcash_rpc() { return zcash_rpc_; }

  MockOrchardSyncState& mocked_sync_state() { return *mocked_sync_state_; }

  base::test::TaskEnvironment& task_environment() { return task_environment_; }

  mojom::AccountIdPtr& account_id() { return account_id_; }

  ZCashWalletService& zcash_wallet_service() { return *zcash_wallet_service_; }

  base::PassKey<ZCashGetChainTipStatusTaskTest> CreatePassKey() {
    return base::PassKey<ZCashGetChainTipStatusTaskTest>();
  }

  KeyringService& keyring_service() { return *keyring_service_; }

 private:
  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;

  base::ScopedTempDir temp_dir_;
  mojom::AccountIdPtr account_id_;
  testing::NiceMock<MockZCashRPC> zcash_rpc_;
  std::unique_ptr<MockOrchardSyncState> mocked_sync_state_;
  base::SequenceBound<OrchardSyncState> sync_state_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<ZCashWalletService> zcash_wallet_service_;
};

TEST_F(ZCashGetChainTipStatusTaskTest, Success) {
  ON_CALL(mocked_sync_state(), GetAccountMeta(_))
      .WillByDefault([](const mojom::AccountIdPtr& account_id) {
        OrchardStorage::AccountMeta meta;
        meta.latest_scanned_block_id = 100;
        return meta;
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
      });

  base::MockCallback<
      ZCashGetZCashChainTipStatusTask::ZCashGetZCashChainTipStatusTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillRepeatedly([&](base::expected<mojom::ZCashChainTipStatusPtr,
                                         std::string> result) {
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(result.value()->latest_scanned_block, 100u);
        EXPECT_EQ(result.value()->chain_tip, 1000u);
      });
  auto task = std::make_unique<ZCashGetZCashChainTipStatusTask>(
      CreatePassKey(), zcash_wallet_service(), CreateContext());

  task->Start(callback.Get());

  task_environment().RunUntilIdle();
}

TEST_F(ZCashGetChainTipStatusTaskTest, EmptyAccount) {
  AccountUtils(&keyring_service())
      .EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
  ON_CALL(mocked_sync_state(), GetAccountMeta(_))
      .WillByDefault(
          [](const mojom::AccountIdPtr& account_id) { return std::nullopt; });

  keyring_service().SetZCashAccountBirthday(
      account_id(), mojom::ZCashAccountShieldBirthday::New(100u, "hash"));

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
      });

  base::MockCallback<
      ZCashGetZCashChainTipStatusTask::ZCashGetZCashChainTipStatusTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillRepeatedly([&](base::expected<mojom::ZCashChainTipStatusPtr,
                                         std::string> result) {
        EXPECT_TRUE(result.has_value());
        EXPECT_EQ(result.value()->latest_scanned_block, 100u);
        EXPECT_EQ(result.value()->chain_tip, 1000u);
      });
  auto task = std::make_unique<ZCashGetZCashChainTipStatusTask>(
      CreatePassKey(), zcash_wallet_service(), CreateContext());

  task->Start(callback.Get());

  task_environment().RunUntilIdle();
}

TEST_F(ZCashGetChainTipStatusTaskTest, Error_AccountNotShielded) {
  ON_CALL(mocked_sync_state(), GetAccountMeta(_))
      .WillByDefault(
          [](const mojom::AccountIdPtr& account_id) { return std::nullopt; });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
      });

  base::MockCallback<
      ZCashGetZCashChainTipStatusTask::ZCashGetZCashChainTipStatusTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillRepeatedly(
          [&](base::expected<mojom::ZCashChainTipStatusPtr, std::string>
                  result) { EXPECT_FALSE(result.has_value()); });
  auto task = std::make_unique<ZCashGetZCashChainTipStatusTask>(
      CreatePassKey(), zcash_wallet_service(), CreateContext());

  task->Start(callback.Get());

  task_environment().RunUntilIdle();
}

TEST_F(ZCashGetChainTipStatusTaskTest, Error_GetAccountMeta) {
  ON_CALL(mocked_sync_state(), GetAccountMeta(_))
      .WillByDefault(
          [](const mojom::AccountIdPtr& account_id) {
            OrchardStorage::AccountMeta meta;
            meta.latest_scanned_block_id = 100;
            return base::unexpected(OrchardStorage::Error{
                OrchardStorage::ErrorCode::kInternalError, ""});
          });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
      });

  base::MockCallback<
      ZCashGetZCashChainTipStatusTask::ZCashGetZCashChainTipStatusTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillRepeatedly(
          [&](base::expected<mojom::ZCashChainTipStatusPtr, std::string>
                  result) { EXPECT_FALSE(result.has_value()); });
  auto task = std::make_unique<ZCashGetZCashChainTipStatusTask>(
      CreatePassKey(), zcash_wallet_service(), CreateContext());

  task->Start(callback.Get());

  task_environment().RunUntilIdle();
}

TEST_F(ZCashGetChainTipStatusTaskTest, Error_GetLatestBlock) {
  ON_CALL(mocked_sync_state(), GetAccountMeta(_))
      .WillByDefault([](const mojom::AccountIdPtr& account_id) {
        OrchardStorage::AccountMeta meta;
        meta.latest_scanned_block_id = 100;
        return meta;
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        std::move(callback).Run(base::unexpected("error"));
      });

  base::MockCallback<
      ZCashGetZCashChainTipStatusTask::ZCashGetZCashChainTipStatusTaskCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_))
      .Times(1)
      .WillRepeatedly(
          [&](base::expected<mojom::ZCashChainTipStatusPtr, std::string>
                  result) { EXPECT_FALSE(result.has_value()); });
  auto task = std::make_unique<ZCashGetZCashChainTipStatusTask>(
      CreatePassKey(), zcash_wallet_service(), CreateContext());

  task->Start(callback.Get());

  task_environment().RunUntilIdle();
}

}  // namespace brave_wallet
