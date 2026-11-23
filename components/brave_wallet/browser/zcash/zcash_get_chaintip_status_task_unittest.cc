/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/internal/orchard_storage/orchard_storage.h"
#include "brave/components/brave_wallet/browser/internal/orchard_sync_state.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_action_context.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_get_zcash_chain_tip_status_task.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/common/features.h"
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

}  // namespace

class ZCashGetChainTipStatusTaskTest : public testing::Test {
 public:
  ZCashGetChainTipStatusTaskTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletZCashFeature,
          {{"zcash_shielded_transactions_enabled", "true"}}},
        },
        {}  // disabled features
    );
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    keyring_service_->Reset();
    keyring_service_->RestoreWallet(kMnemonicGalleryEqual, kTestWalletPassword,
                                    false, base::DoNothing());

    auto account = AccountUtils(keyring_service_.get())
                       .EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
    account_id_ = account->account_id.Clone();

    zcash_wallet_service_ = std::make_unique<TestingZCashWalletService>(
        *keyring_service_, std::make_unique<testing::NiceMock<MockZCashRPC>>());
    zcash_wallet_service_->SetupSyncState(
        OrchardSyncState::CreateSyncStateSequence(),
        std::make_unique<MockOrchardSyncState>(temp_dir_.GetPath()));
  }

  ZCashActionContext CreateContext() {
    return zcash_wallet_service_->CreateActionContext(account_id());
  }

  MockZCashRPC& zcash_rpc() {
    return static_cast<MockZCashRPC&>(zcash_wallet_service_->zcash_rpc());
  }

  MockOrchardSyncState& mocked_sync_state() {
    return static_cast<MockOrchardSyncState&>(
        *zcash_wallet_service_->sync_state_ptr);
  }

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

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<TestingZCashWalletService> zcash_wallet_service_;
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
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        std::move(callback).Run(
            zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
      });

  auto task = std::make_unique<ZCashGetZCashChainTipStatusTask>(
      CreatePassKey(), zcash_wallet_service(), CreateContext());

  base::test::TestFuture<
      base::expected<mojom::ZCashChainTipStatusPtr, std::string>>
      future;
  task->Start(future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value()) << result.error();
  EXPECT_EQ(result.value()->latest_scanned_block, 100u);
  EXPECT_EQ(result.value()->chain_tip, 1000u);
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
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        std::move(callback).Run(
            zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
      });

  auto task = std::make_unique<ZCashGetZCashChainTipStatusTask>(
      CreatePassKey(), zcash_wallet_service(), CreateContext());

  base::test::TestFuture<
      base::expected<mojom::ZCashChainTipStatusPtr, std::string>>
      future;
  task->Start(future.GetCallback());

  auto result = future.Take();
  ASSERT_TRUE(result.has_value()) << result.error();
  EXPECT_EQ(result.value()->latest_scanned_block, 100u);
  EXPECT_EQ(result.value()->chain_tip, 1000u);
}

TEST_F(ZCashGetChainTipStatusTaskTest, Error_AccountNotShielded) {
  ON_CALL(mocked_sync_state(), GetAccountMeta(_))
      .WillByDefault(
          [](const mojom::AccountIdPtr& account_id) { return std::nullopt; });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        std::move(callback).Run(
            zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
      });

  auto task = std::make_unique<ZCashGetZCashChainTipStatusTask>(
      CreatePassKey(), zcash_wallet_service(), CreateContext());

  base::test::TestFuture<
      base::expected<mojom::ZCashChainTipStatusPtr, std::string>>
      future;
  task->Start(future.GetCallback());

  EXPECT_FALSE(future.Take().has_value());
}

TEST_F(ZCashGetChainTipStatusTaskTest, Error_GetAccountMeta) {
  ON_CALL(mocked_sync_state(), GetAccountMeta(_))
      .WillByDefault([](const mojom::AccountIdPtr& account_id) {
        OrchardStorage::AccountMeta meta;
        meta.latest_scanned_block_id = 100;
        return base::unexpected(OrchardStorage::Error{
            OrchardStorage::ErrorCode::kInternalError, ""});
      });

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        ZCashRpc::GetLatestBlockCallback callback) {
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        std::move(callback).Run(
            zcash::mojom::BlockID::New(1000u, std::vector<uint8_t>({})));
      });

  auto task = std::make_unique<ZCashGetZCashChainTipStatusTask>(
      CreatePassKey(), zcash_wallet_service(), CreateContext());

  base::test::TestFuture<
      base::expected<mojom::ZCashChainTipStatusPtr, std::string>>
      future;
  task->Start(future.GetCallback());

  EXPECT_FALSE(future.Take().has_value());
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
        EXPECT_EQ(chain_id, mojom::kZCashMainnet);
        std::move(callback).Run(base::unexpected("error"));
      });

  auto task = std::make_unique<ZCashGetZCashChainTipStatusTask>(
      CreatePassKey(), zcash_wallet_service(), CreateContext());

  base::test::TestFuture<
      base::expected<mojom::ZCashChainTipStatusPtr, std::string>>
      future;
  task->Start(future.GetCallback());

  EXPECT_FALSE(future.Take().has_value());
}

}  // namespace brave_wallet
