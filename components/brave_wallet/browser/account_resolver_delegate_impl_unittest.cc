/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/account_resolver_delegate_impl.h"

#include <memory>
#include <utility>

#include "base/functional/callback_helpers.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-shared.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
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
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    json_rpc_service_ =
        std::make_unique<JsonRpcService>(shared_url_loader_factory_, &prefs_);
    keyring_service_ = std::make_unique<KeyringService>(json_rpc_service_.get(),
                                                        &prefs_, &local_state_);

    keyring_service_->CreateWallet(kMnemonicDivideCruise, "brave",
                                   base::DoNothing());

    resolver_ =
        std::make_unique<AccountResolverDelegateImpl>(keyring_service_.get());
  }

  ~AccountResolverDelegateImplUnitTest() override = default;

  KeyringService* keyring_service() { return keyring_service_.get(); }
  AccountResolverDelegate* resolver() { return resolver_.get(); }

  mojom::AccountInfoPtr AddFilAccount(const std::string& account_name) {
    return keyring_service_->AddAccountSync(
        mojom::CoinType::FIL, mojom::kFilecoinKeyringId, account_name);
  }

  mojom::AccountInfoPtr AddSolAccount(const std::string& account_name) {
    return keyring_service_->AddAccountSync(
        mojom::CoinType::SOL, mojom::kSolanaKeyringId, account_name);
  }

  mojom::AccountInfoPtr AddEthAccount(const std::string& account_name) {
    return keyring_service_->AddAccountSync(
        mojom::CoinType::ETH, mojom::kDefaultKeyringId, account_name);
  }

  mojom::AccountInfoPtr AddBtcAccount(const std::string& account_name) {
    return keyring_service_->AddAccountSync(
        mojom::CoinType::BTC, mojom::kBitcoinKeyring84Id, account_name);
  }

 private:
  base::test::ScopedFeatureList feature_list_{
      features::kBraveWalletBitcoinFeature};
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<AccountResolverDelegateImpl> resolver_;
};

TEST_F(AccountResolverDelegateImplUnitTest, ResolveAccountId) {
  auto some_acc = AddEthAccount("Some acc");

  auto eth_acc = AddEthAccount("Eth acc");
  auto sol_acc = AddSolAccount("Sol acc");
  auto fil_acc = AddFilAccount("Fil acc");
  auto btc_acc = AddBtcAccount("Btc acc");
  ASSERT_TRUE(eth_acc);
  ASSERT_TRUE(sol_acc);
  ASSERT_TRUE(fil_acc);
  ASSERT_TRUE(btc_acc);

  EXPECT_EQ(some_acc->account_id,
            resolver()->ResolveAccountId(nullptr, &some_acc->address));

  for (auto& acc :
       {eth_acc.Clone(), sol_acc.Clone(), fil_acc.Clone(), btc_acc.Clone()}) {
    const auto& account_id = acc->account_id;
    // Resolved by unique_key.
    EXPECT_EQ(account_id,
              resolver()->ResolveAccountId(&account_id->unique_key, nullptr));
    // Resolved by unique_key even if address is provided.
    EXPECT_EQ(account_id, resolver()->ResolveAccountId(&account_id->unique_key,
                                                       &some_acc->address));
    if (account_id->coin != mojom::CoinType::BTC) {
      // Resolved by address.
      EXPECT_EQ(account_id,
                resolver()->ResolveAccountId(nullptr, &acc->address));
    }
  }
  EXPECT_TRUE(AllCoinsTested());

  // Btc acc has no address and should not be resolvable by an empty address.
  EXPECT_EQ("", btc_acc->address);
  EXPECT_EQ("", btc_acc->account_id->address);
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

}  // namespace brave_wallet
