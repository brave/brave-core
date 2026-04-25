/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_auto_sync_manager.h"

#include <memory>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_test_utils.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

using testing::_;

namespace {

class MockZCashWalletService : public TestingZCashWalletService {
 public:
  using TestingZCashWalletService::TestingZCashWalletService;

  MOCK_METHOD2(GetChainTipStatus,
               void(mojom::AccountIdPtr account_id,
                    GetChainTipStatusCallback callback));

  MOCK_METHOD3(StartShieldSync,
               void(mojom::AccountIdPtr account_id,
                    uint32_t to,
                    StartShieldSyncCallback callback));
};

}  // namespace

class ZCashAutoSyncManagerTest : public testing::Test {
 public:
  ZCashAutoSyncManagerTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    ASSERT_TRUE(temp_directory_.CreateUniqueTempDir());
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    mock_zcash_wallet_service_ = std::make_unique<MockZCashWalletService>(
        *keyring_service_, std::make_unique<ZCashRpc>(nullptr, nullptr));
    mock_zcash_wallet_service_->SetupSyncState(
        OrchardSyncState::CreateSyncStateSequence(),
        OrchardSyncState::CreateSyncState(temp_directory_.GetPath()));

    account_id_ = MakeIndexBasedAccountId(mojom::CoinType::ZEC,
                                          mojom::KeyringId::kZCashMainnet,
                                          mojom::AccountKind::kDerived, 0);
    zcash_auto_sync_manager_ = std::make_unique<ZCashAutoSyncManager>(
        *mock_zcash_wallet_service_,
        mock_zcash_wallet_service_->CreateActionContext(account_id_));
  }

  base::test::TaskEnvironment& task_environment() { return task_environment_; }

  MockZCashWalletService& mock_zcash_wallet_service() {
    return *mock_zcash_wallet_service_;
  }

  ZCashAutoSyncManager& zcash_auto_sync_manager() {
    return *zcash_auto_sync_manager_;
  }

 private:
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_directory_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  mojom::AccountIdPtr account_id_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<MockZCashWalletService> mock_zcash_wallet_service_;
  std::unique_ptr<ZCashAutoSyncManager> zcash_auto_sync_manager_;
};

TEST_F(ZCashAutoSyncManagerTest, InitialSync) {
  EXPECT_CALL(mock_zcash_wallet_service(), StartShieldSync(_, _, _));
  ON_CALL(mock_zcash_wallet_service(), GetChainTipStatus(_, _))
      .WillByDefault(
          [](mojom::AccountIdPtr account_id,
             MockZCashWalletService::GetChainTipStatusCallback callback) {
            EXPECT_EQ(GetNetworkForZCashAccount(account_id),
                      mojom::kZCashMainnet);
            std::move(callback).Run(mojom::ZCashChainTipStatus::New(0, 1000),
                                    std::nullopt);
          });

  EXPECT_FALSE(zcash_auto_sync_manager().IsStarted());
  zcash_auto_sync_manager().Start();
  EXPECT_TRUE(zcash_auto_sync_manager().IsStarted());
  task_environment().RunUntilIdle();
}

TEST_F(ZCashAutoSyncManagerTest, TimerHit) {
  EXPECT_CALL(mock_zcash_wallet_service(), StartShieldSync(_, _, _)).Times(2);
  ON_CALL(mock_zcash_wallet_service(), GetChainTipStatus(_, _))
      .WillByDefault(
          [](mojom::AccountIdPtr account_id,
             MockZCashWalletService::GetChainTipStatusCallback callback) {
            EXPECT_EQ(GetNetworkForZCashAccount(account_id),
                      mojom::kZCashMainnet);
            std::move(callback).Run(mojom::ZCashChainTipStatus::New(0, 1000),
                                    std::nullopt);
          });

  EXPECT_FALSE(zcash_auto_sync_manager().IsStarted());
  zcash_auto_sync_manager().Start();
  EXPECT_TRUE(zcash_auto_sync_manager().IsStarted());
  task_environment().RunUntilIdle();
  task_environment().AdvanceClock(base::Minutes(10));
  task_environment().RunUntilIdle();
}

}  // namespace brave_wallet
