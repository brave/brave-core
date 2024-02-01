/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/account_discovery_manager.h"

#include <memory>
#include <string>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_test_utils.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
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

    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    bitcoin_test_rpc_server_ =
        std::make_unique<BitcoinTestRpcServer>(keyring_service_.get(), &prefs_);
    bitcoin_wallet_service_ = std::make_unique<BitcoinWalletService>(
        keyring_service_.get(), &prefs_,
        bitcoin_test_rpc_server_->GetURLLoaderFactory());
    bitcoin_wallet_service_->SetArrangeTransactionsForTesting(true);

    keyring_service_->CreateWallet(kMnemonicDivideCruise, kTestWalletPassword,
                                   base::DoNothing());

    btc_account_ =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kBitcoin84, 0);
    ASSERT_TRUE(btc_account_);
    bitcoin_test_rpc_server_->SetUpBitcoinRpc(btc_account_->account_id);
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  mojom::AccountIdPtr account_id() const {
    return btc_account_->account_id.Clone();
  }

 protected:
  base::test::ScopedFeatureList feature_list_{
      features::kBraveWalletBitcoinFeature};

  mojom::AccountInfoPtr btc_account_;

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  std::unique_ptr<BitcoinTestRpcServer> bitcoin_test_rpc_server_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<BitcoinWalletService> bitcoin_wallet_service_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(AccountDiscoveryManagerUnitTest, DiscoverBtcAccountUpdatesExisting) {
  EXPECT_EQ(*mojom::BitcoinKeyId::New(0, 0),
            *keyring_service_->GetBitcoinAccountInfo(account_id())
                 ->next_receive_address->key_id);
  EXPECT_EQ(*mojom::BitcoinKeyId::New(1, 0),
            *keyring_service_->GetBitcoinAccountInfo(account_id())
                 ->next_change_address->key_id);

  AccountDiscoveryManager discovery_manager(nullptr, keyring_service_.get(),
                                            bitcoin_wallet_service_.get());
  discovery_manager.StartDiscovery();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(*mojom::BitcoinKeyId::New(0, 1),
            *keyring_service_->GetBitcoinAccountInfo(account_id())
                 ->next_receive_address->key_id);
  EXPECT_EQ(*mojom::BitcoinKeyId::New(1, 1),
            *keyring_service_->GetBitcoinAccountInfo(account_id())
                 ->next_change_address->key_id);
}

}  // namespace brave_wallet
