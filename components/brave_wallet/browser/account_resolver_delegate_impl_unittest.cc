/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/account_resolver_delegate_impl.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class AccountResolverDelegateImplUnitTest : public testing::Test {
 public:
  AccountResolverDelegateImplUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {
    feature_list_.InitWithFeatures({features::kBraveWalletBitcoinFeature,
                                    features::kBraveWalletBitcoinLedgerFeature,
                                    features::kBraveWalletZCashFeature},
                                   {});
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    json_rpc_service_ = std::make_unique<JsonRpcService>(
        shared_url_loader_factory_, network_manager_.get(), &prefs_, nullptr);
    keyring_service_ = std::make_unique<KeyringService>(json_rpc_service_.get(),
                                                        &prefs_, &local_state_);

    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);

    resolver_ =
        std::make_unique<AccountResolverDelegateImpl>(*keyring_service_);
  }

  ~AccountResolverDelegateImplUnitTest() override = default;

  KeyringService* keyring_service() { return keyring_service_.get(); }
  AccountResolverDelegate* resolver() { return resolver_.get(); }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<AccountResolverDelegateImpl> resolver_;
};

TEST_F(AccountResolverDelegateImplUnitTest, ResolveAccountId) {
  std::vector<mojom::AccountInfoPtr> accounts;
  for (const auto& keyring_id : kAllKeyrings) {
    auto account = GetAccountUtils().EnsureAccount(keyring_id, 0);
    accounts.push_back(std::move(account));
  }

  auto hw_eth_acc = GetAccountUtils().CreateEthHWAccount();
  accounts.push_back(hw_eth_acc.Clone());

  auto hw_btc_acc = GetAccountUtils().CreateBtcHWAccount();
  accounts.push_back(hw_btc_acc.Clone());

  auto some_acc = GetAccountUtils().EnsureEthAccount(1);
  accounts.push_back(some_acc.Clone());

  for (const auto& acc : accounts) {
    ASSERT_TRUE(acc);
    const auto& account_id = acc->account_id;
    // Resolved by unique_key.
    EXPECT_EQ(account_id,
              resolver()->ResolveAccountId(&account_id->unique_key, nullptr));
    EXPECT_EQ(account_id, resolver()->ResolveAccountId(&account_id->unique_key,
                                                       &account_id->address));
    // Resolved by unique_key even if address is provided.
    EXPECT_EQ(account_id, resolver()->ResolveAccountId(&account_id->unique_key,
                                                       &some_acc->address));
    if (account_id->coin != mojom::CoinType::BTC &&
        account_id->coin != mojom::CoinType::ZEC) {
      // Resolved by address.
      EXPECT_EQ(account_id,
                resolver()->ResolveAccountId(nullptr, &acc->address));
    }
  }
  EXPECT_TRUE(AllCoinsTested());
  EXPECT_TRUE(AllKeyringsTested());

  // HW account is not resolvable after removal.
  keyring_service()->RemoveAccount(hw_eth_acc->account_id.Clone(),
                                   kTestWalletPassword, base::DoNothing());
  EXPECT_FALSE(resolver()->ResolveAccountId(&hw_eth_acc->account_id->unique_key,
                                            nullptr));
  EXPECT_FALSE(resolver()->ResolveAccountId(&hw_eth_acc->account_id->unique_key,
                                            &hw_eth_acc->address));
  EXPECT_FALSE(resolver()->ResolveAccountId(nullptr, &hw_eth_acc->address));

  keyring_service()->RemoveAccount(hw_btc_acc->account_id.Clone(),
                                   kTestWalletPassword, base::DoNothing());
  EXPECT_FALSE(resolver()->ResolveAccountId(&hw_btc_acc->account_id->unique_key,
                                            nullptr));

  // Btc-like accs have no address and should not be resolvable by an empty
  // address.
  for (const auto& keyring_id :
       {mojom::KeyringId::kBitcoin84, mojom::KeyringId::kBitcoin84Testnet,
        mojom::KeyringId::kBitcoinImport,
        mojom::KeyringId::kBitcoinImportTestnet,
        mojom::KeyringId::kBitcoinHardware,
        mojom::KeyringId::kBitcoinHardwareTestnet,
        mojom::KeyringId::kZCashMainnet, mojom::KeyringId::kBitcoin84Testnet}) {
    auto btc_like_account = GetAccountUtils().EnsureAccount(keyring_id, 0);
    EXPECT_EQ("", btc_like_account->address);
    EXPECT_EQ("", btc_like_account->account_id->address);
  }
  EXPECT_TRUE(AllCoinsTested());
  EXPECT_TRUE(AllKeyringsTested());

  const std::string empty_address = "";
  EXPECT_FALSE(resolver()->ResolveAccountId(nullptr, &empty_address));
  EXPECT_FALSE(resolver()->ResolveAccountId(&empty_address, &empty_address));
  EXPECT_FALSE(resolver()->ResolveAccountId(&empty_address, nullptr));

  const std::string unknown_address = "unknown_address";
  EXPECT_FALSE(resolver()->ResolveAccountId(nullptr, nullptr));
  EXPECT_FALSE(resolver()->ResolveAccountId(nullptr, &unknown_address));
  EXPECT_FALSE(
      resolver()->ResolveAccountId(&unknown_address, &unknown_address));
  EXPECT_FALSE(resolver()->ResolveAccountId(&unknown_address, nullptr));
}

TEST_F(AccountResolverDelegateImplUnitTest, ValidateAccountId) {
  std::vector<mojom::AccountInfoPtr> accounts;
  for (const auto& keyring_id : kAllKeyrings) {
    auto account = GetAccountUtils().EnsureAccount(keyring_id, 0);
    accounts.push_back(std::move(account));
  }

  auto hw_eth_acc = GetAccountUtils().CreateEthHWAccount();
  accounts.push_back(hw_eth_acc.Clone());
  auto hw_btc_acc = GetAccountUtils().CreateBtcHWAccount();
  accounts.push_back(hw_btc_acc.Clone());

  for (const auto& acc : accounts) {
    ASSERT_TRUE(acc);
    EXPECT_TRUE(resolver()->ValidateAccountId(acc->account_id));
  }
  EXPECT_TRUE(AllCoinsTested());
  EXPECT_TRUE(AllKeyringsTested());

  EXPECT_FALSE(
      resolver()->ValidateAccountId(GetAccountUtils().EthUnkownAccountId()));

  // HW accounts are invalid after removal.
  keyring_service()->RemoveAccount(hw_eth_acc->account_id.Clone(),
                                   kTestWalletPassword, base::DoNothing());
  EXPECT_FALSE(resolver()->ValidateAccountId(hw_eth_acc->account_id));
  keyring_service()->RemoveAccount(hw_btc_acc->account_id.Clone(),
                                   kTestWalletPassword, base::DoNothing());
  EXPECT_FALSE(resolver()->ValidateAccountId(hw_btc_acc->account_id));
}

}  // namespace brave_wallet
