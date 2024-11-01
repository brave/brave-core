/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/account_discovery_manager.h"

#include <memory>
#include <string>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_hd_keyring.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_test_utils.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class AccountDiscoveryManagerUnitTest : public testing::Test {
 public:
  AccountDiscoveryManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  ~AccountDiscoveryManagerUnitTest() override = default;

  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    bitcoin_test_rpc_server_ = std::make_unique<BitcoinTestRpcServer>();
    bitcoin_wallet_service_ = std::make_unique<BitcoinWalletService>(
        *keyring_service_, *network_manager_,
        bitcoin_test_rpc_server_->GetURLLoaderFactory());
    bitcoin_wallet_service_->SetArrangeTransactionsForTesting(true);

    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);

    bitcoin_test_rpc_server_->SetUpBitcoinRpc(std::nullopt, std::nullopt);

    keyring_ = std::make_unique<BitcoinHDKeyring>(
        *MnemonicToSeed(kMnemonicDivideCruise), false);
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  BitcoinHDKeyring* keyring() { return keyring_.get(); }

 protected:
  base::test::ScopedFeatureList feature_list_{
      features::kBraveWalletBitcoinFeature};

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  std::unique_ptr<BitcoinTestRpcServer> bitcoin_test_rpc_server_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<BitcoinWalletService> bitcoin_wallet_service_;
  std::unique_ptr<BitcoinHDKeyring> keyring_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(AccountDiscoveryManagerUnitTest, DiscoverBtcAccountCreatesNew) {
  bitcoin_test_rpc_server_->AddTransactedAddress(
      keyring()->GetAddress(0, {0, 10}));
  bitcoin_test_rpc_server_->AddTransactedAddress(
      keyring()->GetAddress(0, {1, 15}));
  bitcoin_test_rpc_server_->AddTransactedAddress(
      keyring()->GetAddress(1, {0, 19}));

  EXPECT_EQ(0u, GetAccountUtils().AllBtcAccounts().size());

  AccountDiscoveryManager discovery_manager(nullptr, keyring_service_.get(),
                                            bitcoin_wallet_service_.get());
  discovery_manager.StartDiscovery();
  task_environment_.RunUntilIdle();

  EXPECT_EQ(2u, GetAccountUtils().AllBtcAccounts().size());
  auto account_0 = GetAccountUtils().AllBtcAccounts()[0]->account_id->Clone();
  auto account_1 = GetAccountUtils().AllBtcAccounts()[1]->account_id->Clone();

  EXPECT_EQ(*mojom::BitcoinKeyId::New(0, 10 + 1),
            *keyring_service_->GetBitcoinAccountInfo(account_0)
                 ->next_receive_address->key_id);
  EXPECT_EQ(*mojom::BitcoinKeyId::New(1, 15 + 1),
            *keyring_service_->GetBitcoinAccountInfo(account_0)
                 ->next_change_address->key_id);

  EXPECT_EQ(*mojom::BitcoinKeyId::New(0, 19 + 1),
            *keyring_service_->GetBitcoinAccountInfo(account_1)
                 ->next_receive_address->key_id);
  EXPECT_EQ(*mojom::BitcoinKeyId::New(1, 0),
            *keyring_service_->GetBitcoinAccountInfo(account_1)
                 ->next_change_address->key_id);
}

TEST_F(AccountDiscoveryManagerUnitTest, DiscoverBtcAccountUpdatesExisting) {
  bitcoin_test_rpc_server_->AddTransactedAddress(
      keyring()->GetAddress(0, {0, 10}));
  bitcoin_test_rpc_server_->AddTransactedAddress(
      keyring()->GetAddress(0, {1, 15}));

  auto account_id = GetAccountUtils()
                        .EnsureAccount(mojom::KeyringId::kBitcoin84, 0)
                        ->account_id->Clone();

  EXPECT_EQ(*mojom::BitcoinKeyId::New(0, 0),
            *keyring_service_->GetBitcoinAccountInfo(account_id)
                 ->next_receive_address->key_id);
  EXPECT_EQ(*mojom::BitcoinKeyId::New(1, 0),
            *keyring_service_->GetBitcoinAccountInfo(account_id)
                 ->next_change_address->key_id);

  AccountDiscoveryManager discovery_manager(nullptr, keyring_service_.get(),
                                            bitcoin_wallet_service_.get());
  discovery_manager.StartDiscovery();
  task_environment_.RunUntilIdle();

  EXPECT_EQ(*mojom::BitcoinKeyId::New(0, 10 + 1),
            *keyring_service_->GetBitcoinAccountInfo(account_id)
                 ->next_receive_address->key_id);
  EXPECT_EQ(*mojom::BitcoinKeyId::New(1, 15 + 1),
            *keyring_service_->GetBitcoinAccountInfo(account_id)
                 ->next_change_address->key_id);
}

}  // namespace brave_wallet
