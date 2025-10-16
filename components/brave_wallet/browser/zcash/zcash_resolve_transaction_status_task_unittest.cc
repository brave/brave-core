/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_resolve_transaction_status_task.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_action_context.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_rpc.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_transaction.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_tx_meta.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/services/brave_wallet/public/mojom/zcash_decoder.mojom.h"
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

constexpr uint32_t kTransactionHeight = 10;

class MockZCashRPC : public ZCashRpc {
 public:
  MockZCashRPC() : ZCashRpc(nullptr, nullptr) {}
  ~MockZCashRPC() override = default;

  MOCK_METHOD2(GetLatestBlock,
               void(const std::string& chain_id,
                    GetLatestBlockCallback callback));

  MOCK_METHOD3(GetTransaction,
               void(const std::string& chain_id,
                    const std::string& tx_hash,
                    GetTransactionCallback callback));
};

}  // namespace

class ZCashResolveTransactionStatusTaskTest : public testing::Test {
 public:
  ZCashResolveTransactionStatusTaskTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    base::FilePath db_path(
        temp_dir_.GetPath().Append(FILE_PATH_LITERAL("orchard.db")));

    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    keyring_service_->Reset();
    keyring_service_->RestoreWallet(kMnemonicGalleryEqual, kTestWalletPassword,
                                    false, base::DoNothing());

    auto account = AccountUtils(keyring_service_.get())
                       .EnsureAccount(mojom::KeyringId::kZCashMainnet, 0);
    account_id_ = account->account_id.Clone();

    zcash_wallet_service_ = std::make_unique<ZCashWalletService>(
        db_path, *keyring_service_,
        std::make_unique<testing::NiceMock<ZCashRpc>>(nullptr, nullptr));
  }

  testing::NiceMock<MockZCashRPC>& zcash_rpc() { return zcash_rpc_; }

  base::test::TaskEnvironment& task_environment() { return task_environment_; }

  mojom::AccountIdPtr& account_id() { return account_id_; }

  std::unique_ptr<ZCashTxMeta> CreateZCashTxMeta() {
    std::unique_ptr<ZCashTransaction> zcash_transaction =
        std::make_unique<ZCashTransaction>();
    return std::make_unique<ZCashTxMeta>(account_id(),
                                         std::move(zcash_transaction));
  }

  ZCashWalletService& zcash_wallet_service() { return *zcash_wallet_service_; }

  ZCashActionContext CreateContext() {
    return ZCashActionContext(zcash_rpc_,
#if BUILDFLAG(ENABLE_ORCHARD)
                              {}, sync_state_,
#endif
                              account_id_);
  }

  base::PassKey<class ZCashResolveTransactionStatusTaskTest> CreatePassKey() {
    return base::PassKey<class ZCashResolveTransactionStatusTaskTest>();
  }

 private:
  base::test::TaskEnvironment task_environment_;
#if BUILDFLAG(ENABLE_ORCHARD)
  base::SequenceBound<OrchardSyncState> sync_state_;
#endif

  base::ScopedTempDir temp_dir_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<ZCashWalletService> zcash_wallet_service_;

  mojom::AccountIdPtr account_id_;
  testing::NiceMock<MockZCashRPC> zcash_rpc_;
};

TEST_F(ZCashResolveTransactionStatusTaskTest, Confirmed) {
  auto tx_meta = CreateZCashTxMeta();
  tx_meta->tx()->set_expiry_height(10);
  tx_meta->set_tx_hash("tx_hash");

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault(
          [](const std::string& chain_id,
             MockZCashRPC::GetLatestBlockCallback callback) {
            EXPECT_EQ(chain_id, mojom::kZCashMainnet);
            std::move(callback).Run(zcash::mojom::BlockID::New(
                kTransactionHeight + 5u, std::vector<uint8_t>()));
          });

  ON_CALL(zcash_rpc(), GetTransaction(_, _, _))
      .WillByDefault(
          [](const std::string& chain_id, const std::string& tx_hash,
             MockZCashRPC::GetTransactionCallback callback) {
            EXPECT_EQ(chain_id, mojom::kZCashMainnet);
            EXPECT_EQ(tx_hash, "tx_hash");
            std::move(callback).Run(zcash::mojom::RawTransaction::New(
                std::vector<uint8_t>(), kTransactionHeight));
          });

  base::MockCallback<ZCashResolveTransactionStatusTask::
                         ZCashResolveTransactionStatusTaskCallback>
      callback;

  auto task = std::make_unique<ZCashResolveTransactionStatusTask>(
      CreatePassKey(), CreateContext(), zcash_wallet_service(),
      std::move(tx_meta));
  base::expected<ZCashWalletService::ResolveTransactionStatusResult,
                 std::string>
      tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start(callback.Get());

  task_environment().RunUntilIdle();

  EXPECT_EQ(tx_result.value(),
            ZCashWalletService::ResolveTransactionStatusResult::kCompleted);
}

TEST_F(ZCashResolveTransactionStatusTaskTest, Expired_ExpiryHeight) {
  auto tx_meta = CreateZCashTxMeta();
  tx_meta->tx()->set_expiry_height(15);
  tx_meta->set_tx_hash("tx_hash");

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        MockZCashRPC::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            zcash::mojom::BlockID::New(20u, std::vector<uint8_t>()));
      });

  ON_CALL(zcash_rpc(), GetTransaction(_, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& tx_hash,
                        MockZCashRPC::GetTransactionCallback callback) {
        EXPECT_EQ(tx_hash, "tx_hash");
        std::move(callback).Run(
            zcash::mojom::RawTransaction::New(std::vector<uint8_t>(), 0));
      });

  base::MockCallback<ZCashResolveTransactionStatusTask::
                         ZCashResolveTransactionStatusTaskCallback>
      callback;

  auto task = std::make_unique<ZCashResolveTransactionStatusTask>(
      CreatePassKey(), CreateContext(), zcash_wallet_service(),
      std::move(tx_meta));
  base::expected<ZCashWalletService::ResolveTransactionStatusResult,
                 std::string>
      tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start(callback.Get());

  task_environment().RunUntilIdle();

  EXPECT_EQ(tx_result.value(),
            ZCashWalletService::ResolveTransactionStatusResult::kExpired);
}

TEST_F(ZCashResolveTransactionStatusTaskTest, Expired_Time) {
  auto tx_meta = CreateZCashTxMeta();
  tx_meta->set_submitted_time(base::Time::Now());
  task_environment().AdvanceClock(base::Hours(3));
  tx_meta->set_tx_hash("tx_hash");

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        MockZCashRPC::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            zcash::mojom::BlockID::New(20u, std::vector<uint8_t>()));
      });

  ON_CALL(zcash_rpc(), GetTransaction(_, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& tx_hash,
                        MockZCashRPC::GetTransactionCallback callback) {
        EXPECT_EQ(tx_hash, "tx_hash");
        std::move(callback).Run(
            zcash::mojom::RawTransaction::New(std::vector<uint8_t>(), 0));
      });

  base::MockCallback<ZCashResolveTransactionStatusTask::
                         ZCashResolveTransactionStatusTaskCallback>
      callback;

  auto task = std::make_unique<ZCashResolveTransactionStatusTask>(
      CreatePassKey(), CreateContext(), zcash_wallet_service(),
      std::move(tx_meta));
  base::expected<ZCashWalletService::ResolveTransactionStatusResult,
                 std::string>
      tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start(callback.Get());

  task_environment().RunUntilIdle();

  EXPECT_EQ(tx_result.value(),
            ZCashWalletService::ResolveTransactionStatusResult::kExpired);
}

TEST_F(ZCashResolveTransactionStatusTaskTest, InProgress_ExpiryHeight) {
  auto tx_meta = CreateZCashTxMeta();
  tx_meta->tx()->set_expiry_height(15);
  tx_meta->set_tx_hash("tx_hash");

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        MockZCashRPC::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            zcash::mojom::BlockID::New(12u, std::vector<uint8_t>()));
      });

  ON_CALL(zcash_rpc(), GetTransaction(_, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& tx_hash,
                        MockZCashRPC::GetTransactionCallback callback) {
        EXPECT_EQ(tx_hash, "tx_hash");
        std::move(callback).Run(
            zcash::mojom::RawTransaction::New(std::vector<uint8_t>(), 0));
      });

  base::MockCallback<ZCashResolveTransactionStatusTask::
                         ZCashResolveTransactionStatusTaskCallback>
      callback;

  auto task = std::make_unique<ZCashResolveTransactionStatusTask>(
      CreatePassKey(), CreateContext(), zcash_wallet_service(),
      std::move(tx_meta));
  base::expected<ZCashWalletService::ResolveTransactionStatusResult,
                 std::string>
      tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start(callback.Get());

  task_environment().RunUntilIdle();

  EXPECT_EQ(tx_result.value(),
            ZCashWalletService::ResolveTransactionStatusResult::kInProgress);
}

TEST_F(ZCashResolveTransactionStatusTaskTest, InProgress_Time) {
  auto tx_meta = CreateZCashTxMeta();
  tx_meta->set_submitted_time(base::Time::Now());
  task_environment().AdvanceClock(base::Hours(1));
  tx_meta->set_tx_hash("tx_hash");

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        MockZCashRPC::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            zcash::mojom::BlockID::New(12u, std::vector<uint8_t>()));
      });

  ON_CALL(zcash_rpc(), GetTransaction(_, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& tx_hash,
                        MockZCashRPC::GetTransactionCallback callback) {
        EXPECT_EQ(tx_hash, "tx_hash");
        std::move(callback).Run(
            zcash::mojom::RawTransaction::New(std::vector<uint8_t>(), 0));
      });

  base::MockCallback<ZCashResolveTransactionStatusTask::
                         ZCashResolveTransactionStatusTaskCallback>
      callback;

  auto task = std::make_unique<ZCashResolveTransactionStatusTask>(
      CreatePassKey(), CreateContext(), zcash_wallet_service(),
      std::move(tx_meta));
  base::expected<ZCashWalletService::ResolveTransactionStatusResult,
                 std::string>
      tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start(callback.Get());

  task_environment().RunUntilIdle();

  EXPECT_EQ(tx_result.value(),
            ZCashWalletService::ResolveTransactionStatusResult::kInProgress);
}

TEST_F(ZCashResolveTransactionStatusTaskTest,
       InProgress_Time_NowIsLessThanSubmitted) {
  auto tx_meta = CreateZCashTxMeta();
  tx_meta->set_submitted_time(base::Time::Now() + base::Hours(4));
  tx_meta->set_tx_hash("tx_hash");

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        MockZCashRPC::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            zcash::mojom::BlockID::New(12u, std::vector<uint8_t>()));
      });

  ON_CALL(zcash_rpc(), GetTransaction(_, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& tx_hash,
                        MockZCashRPC::GetTransactionCallback callback) {
        EXPECT_EQ(tx_hash, "tx_hash");
        std::move(callback).Run(
            zcash::mojom::RawTransaction::New(std::vector<uint8_t>(), 0));
      });

  base::MockCallback<ZCashResolveTransactionStatusTask::
                         ZCashResolveTransactionStatusTaskCallback>
      callback;

  auto task = std::make_unique<ZCashResolveTransactionStatusTask>(
      CreatePassKey(), CreateContext(), zcash_wallet_service(),
      std::move(tx_meta));
  base::expected<ZCashWalletService::ResolveTransactionStatusResult,
                 std::string>
      tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start(callback.Get());

  task_environment().RunUntilIdle();

  EXPECT_EQ(tx_result.value(),
            ZCashWalletService::ResolveTransactionStatusResult::kInProgress);
}

TEST_F(ZCashResolveTransactionStatusTaskTest, Error_Transaction) {
  auto tx_meta = CreateZCashTxMeta();
  tx_meta->set_tx_hash("tx_hash");

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        MockZCashRPC::GetLatestBlockCallback callback) {
        std::move(callback).Run(
            zcash::mojom::BlockID::New(12u, std::vector<uint8_t>()));
      });

  ON_CALL(zcash_rpc(), GetTransaction(_, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& tx_hash,
                        MockZCashRPC::GetTransactionCallback callback) {
        EXPECT_EQ(tx_hash, "tx_hash");
        std::move(callback).Run(base::unexpected("error"));
      });

  base::MockCallback<ZCashResolveTransactionStatusTask::
                         ZCashResolveTransactionStatusTaskCallback>
      callback;

  auto task = std::make_unique<ZCashResolveTransactionStatusTask>(
      CreatePassKey(), CreateContext(), zcash_wallet_service(),
      std::move(tx_meta));
  base::expected<ZCashWalletService::ResolveTransactionStatusResult,
                 std::string>
      tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start(callback.Get());

  task_environment().RunUntilIdle();

  EXPECT_FALSE(tx_result.has_value());
}

TEST_F(ZCashResolveTransactionStatusTaskTest, Error_LatestBlock) {
  auto tx_meta = CreateZCashTxMeta();
  tx_meta->set_tx_hash("tx_hash");

  ON_CALL(zcash_rpc(), GetLatestBlock(_, _))
      .WillByDefault([](const std::string& chain_id,
                        MockZCashRPC::GetLatestBlockCallback callback) {
        std::move(callback).Run(base::unexpected("error"));
      });

  ON_CALL(zcash_rpc(), GetTransaction(_, _, _))
      .WillByDefault([](const std::string& chain_id, const std::string& tx_hash,
                        MockZCashRPC::GetTransactionCallback callback) {
        EXPECT_EQ(tx_hash, "tx_hash");
        std::move(callback).Run(
            zcash::mojom::RawTransaction::New(std::vector<uint8_t>(), 0));
      });

  base::MockCallback<ZCashResolveTransactionStatusTask::
                         ZCashResolveTransactionStatusTaskCallback>
      callback;

  auto task = std::make_unique<ZCashResolveTransactionStatusTask>(
      CreatePassKey(), CreateContext(), zcash_wallet_service(),
      std::move(tx_meta));
  base::expected<ZCashWalletService::ResolveTransactionStatusResult,
                 std::string>
      tx_result;
  EXPECT_CALL(callback, Run(_)).WillOnce(SaveArg<0>(&tx_result));

  task->Start(callback.Get());

  task_environment().RunUntilIdle();

  EXPECT_FALSE(tx_result.has_value());
}

}  // namespace brave_wallet
