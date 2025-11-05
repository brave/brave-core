/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_tx_manager.h"

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/account_resolver_delegate_impl.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/browser/tx_storage_delegate_impl.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class PolkadotTxManagerUnitTest : public testing::Test {
 public:
  PolkadotTxManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  void SetUp() override {
    RegisterProfilePrefs(profile_prefs_.registry());
    RegisterLocalStatePrefs(local_state_.registry());
    RegisterProfilePrefsForMigration(profile_prefs_.registry());

    network_manager_ = std::make_unique<NetworkManager>(&profile_prefs_);
    json_rpc_service_ = std::make_unique<JsonRpcService>(
        shared_url_loader_factory_, network_manager_.get(), &profile_prefs_,
        &local_state_);
    keyring_service_ = std::make_unique<KeyringService>(
        json_rpc_service_.get(), &profile_prefs_, &local_state_);

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    tx_service_ = std::make_unique<TxService>(
        json_rpc_service_.get(), nullptr, nullptr, nullptr, *keyring_service_,
        &profile_prefs_, temp_dir_.GetPath(),
        task_environment_.GetMainThreadTaskRunner());

    account_resolver_delegate_ =
        std::make_unique<AccountResolverDelegateImpl>(*keyring_service_);

    polkadot_tx_manager_ = std::make_unique<PolkadotTxManager>(
        *tx_service_, *keyring_service_, *tx_service_->GetDelegateForTesting(),
        *account_resolver_delegate_);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;

  sync_preferences::TestingPrefServiceSyncable profile_prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  base::ScopedTempDir temp_dir_;

  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<TxService> tx_service_;
  std::unique_ptr<AccountResolverDelegateImpl> account_resolver_delegate_;
  std::unique_ptr<PolkadotTxManager> polkadot_tx_manager_;
};

TEST_F(PolkadotTxManagerUnitTest, GetCoinType) {
  EXPECT_EQ(polkadot_tx_manager_->GetCoinType(), mojom::CoinType::DOT);
}

TEST_F(PolkadotTxManagerUnitTest, AddUnapprovedTransaction) {
  auto tx_data_union =
      mojom::TxDataUnion::NewPolkadotTxData(mojom::PolkadotTxdata::New(""));

  auto account_id = mojom::AccountId::New();
  account_id->coin = mojom::CoinType::DOT;
  account_id->keyring_id = mojom::KeyringId::kPolkadotMainnet;
  account_id->kind = mojom::AccountKind::kDerived;
  account_id->address = "test_address";

  polkadot_tx_manager_->AddUnapprovedTransaction(
      "polkadot_mainnet", std::move(tx_data_union), account_id, std::nullopt,
      base::BindOnce([](bool success, const std::string& tx_meta_id,
                        const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(tx_meta_id.empty());
        EXPECT_EQ(error_message, "Not implemented");
      }));
}

TEST_F(PolkadotTxManagerUnitTest, ApproveTransaction) {
  polkadot_tx_manager_->ApproveTransaction(
      "test_tx_id",
      base::BindOnce([](bool success, mojom::ProviderErrorUnionPtr error_union,
                        const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(error_union);
        EXPECT_EQ(error_message, "Not implemented");
      }));
}

TEST_F(PolkadotTxManagerUnitTest, SpeedupOrCancelTransaction) {
  polkadot_tx_manager_->SpeedupOrCancelTransaction(
      "test_tx_id", false,  // false = speedup, true = cancel
      base::BindOnce([](bool success, const std::string& tx_meta_id,
                        const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(tx_meta_id.empty());
        EXPECT_EQ(error_message, "Not implemented");
      }));
}

TEST_F(PolkadotTxManagerUnitTest, RetryTransaction) {
  polkadot_tx_manager_->RetryTransaction(
      "test_tx_id",
      base::BindOnce([](bool success, const std::string& tx_meta_id,
                        const std::string& error_message) {
        EXPECT_FALSE(success);
        EXPECT_TRUE(tx_meta_id.empty());
        EXPECT_EQ(error_message, "Not implemented");
      }));
}

}  // namespace brave_wallet
