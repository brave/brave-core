/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_dapp_utils.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"  // IWYU pragma: keep
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

constexpr char kSr25519[] = "sr25519";

}  // namespace

class PolkadotDappUtilsUnitTest : public testing::Test {
 public:
  PolkadotDappUtilsUnitTest() {
    feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletPolkadotFeature,
          {{"polkadot_dapp_support", "true"}}}},
        {});
  }

  void SetUp() override {
    RegisterLocalStatePrefs(local_state_.registry());
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());

    brave_wallet_service_ = std::make_unique<BraveWalletService>(
        url_loader_factory_.GetSafeWeakWrapper(),
        TestBraveWalletServiceDelegate::Create(), &prefs_, &local_state_);
  }

  void CreateWallet() {
    AccountUtils(keyring_service())
        .CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
  }

  mojom::AccountInfoPtr AddAccount(
      mojom::KeyringId keyring_id = mojom::KeyringId::kPolkadotMainnet,
      uint32_t index = 0) {
    return AccountUtils(keyring_service()).EnsureAccount(keyring_id, index);
  }

  void SelectAccount(const mojom::AccountInfoPtr& account) {
    ASSERT_TRUE(
        keyring_service()->SetSelectedAccountSync(account->account_id.Clone()));
  }

  std::string PermissionIdentifier(const mojom::AccountInfoPtr& account) {
    return GetAccountPermissionIdentifier(account->account_id);
  }

  KeyringService* keyring_service() {
    return brave_wallet_service_->keyring_service();
  }

  sync_preferences::TestingPrefServiceSyncable* prefs() { return &prefs_; }

 private:
  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<BraveWalletService> brave_wallet_service_;
};

TEST_F(PolkadotDappUtilsUnitTest, PermissionIdentifiers_NoWallet) {
  EXPECT_THAT(GetPolkadotAccountPermissionIdentifiers(keyring_service()),
              testing::IsEmpty());
}

TEST_F(PolkadotDappUtilsUnitTest, PermissionIdentifiers_NoPolkadotAccounts) {
  // DOT accounts aren't created by default.

  CreateWallet();

  EXPECT_THAT(GetPolkadotAccountPermissionIdentifiers(keyring_service()),
              testing::IsEmpty());
}

TEST_F(PolkadotDappUtilsUnitTest, PermissionIdentifiers_MainnetAndImported) {
  CreateWallet();
  auto derived = AddAccount(mojom::KeyringId::kPolkadotMainnet);
  auto imported = AddAccount(mojom::KeyringId::kPolkadotImport);
  ASSERT_TRUE(derived);
  ASSERT_TRUE(imported);

  // Testnet and mainnet accounts share keypairs, so exposing a testnet account
  // to a dapp would give a false sense of separation. See
  // `PolkadotProviderImpl`.
  ASSERT_TRUE(AddAccount(mojom::KeyringId::kPolkadotTestnet));
  ASSERT_TRUE(AddAccount(mojom::KeyringId::kPolkadotImportTestnet));

  EXPECT_THAT(GetPolkadotAccountPermissionIdentifiers(keyring_service()),
              testing::UnorderedElementsAre(PermissionIdentifier(derived),
                                            PermissionIdentifier(imported)));
}

TEST_F(PolkadotDappUtilsUnitTest, PreferredAccount_NoAllowedAccounts) {
  CreateWallet();
  ASSERT_TRUE(AddAccount());

  EXPECT_FALSE(GetPolkadotPreferredDappAccount(
      keyring_service(), /*allowed_accounts=*/std::nullopt));
  EXPECT_FALSE(GetPolkadotPreferredDappAccount(
      keyring_service(),
      /*allowed_accounts=*/std::vector<std::string>{}));
}

TEST_F(PolkadotDappUtilsUnitTest, PreferredAccount_SelectedAccountWins) {
  CreateWallet();
  auto first = AddAccount(mojom::KeyringId::kPolkadotMainnet, 0);
  auto second = AddAccount(mojom::KeyringId::kPolkadotMainnet, 1);
  ASSERT_TRUE(first);
  ASSERT_TRUE(second);
  SelectAccount(second);

  ASSERT_EQ(prefs()
                ->FindPreference(kBraveWalletSelectedDotDappAccount)
                ->GetValue()
                ->GetString(),
            PermissionIdentifier(second));

  EXPECT_THAT(GetPolkadotPreferredDappAccount(
                  keyring_service(),
                  std::vector<std::string>{PermissionIdentifier(first),
                                           PermissionIdentifier(second)}),
              EqualsMojo(second->account_id));

  auto imported = AddAccount(mojom::KeyringId::kPolkadotImport);
  ASSERT_TRUE(imported);
  SelectAccount(imported);

  ASSERT_EQ(prefs()
                ->FindPreference(kBraveWalletSelectedDotDappAccount)
                ->GetValue()
                ->GetString(),
            PermissionIdentifier(imported));

  EXPECT_THAT(GetPolkadotPreferredDappAccount(
                  keyring_service(),
                  std::vector<std::string>{PermissionIdentifier(imported)}),
              EqualsMojo(imported->account_id));
}

TEST_F(PolkadotDappUtilsUnitTest,
       PreferredAccount_SelectedNotAllowedFallsBack) {
  CreateWallet();
  auto first = AddAccount(mojom::KeyringId::kPolkadotMainnet, 0);
  auto second = AddAccount(mojom::KeyringId::kPolkadotMainnet, 1);
  ASSERT_TRUE(first);
  ASSERT_TRUE(second);
  SelectAccount(second);

  EXPECT_THAT(GetPolkadotPreferredDappAccount(
                  keyring_service(),
                  std::vector<std::string>{PermissionIdentifier(first)}),
              EqualsMojo(first->account_id));
}

TEST_F(PolkadotDappUtilsUnitTest, PreferredAccount_NoSelectionFallsBack) {
  // Adding the testnet account selects it for DOT, and
  // `GetSelectedPolkadotDappAccount` only looks at the mainnet and import
  // keyrings, so there is no selected dapp account to prefer.

  CreateWallet();
  auto mainnet = AddAccount(mojom::KeyringId::kPolkadotMainnet);
  ASSERT_TRUE(mainnet);

  ASSERT_TRUE(AddAccount(mojom::KeyringId::kPolkadotTestnet));
  ASSERT_FALSE(keyring_service()->GetSelectedPolkadotDappAccount());

  EXPECT_THAT(GetPolkadotPreferredDappAccount(
                  keyring_service(),
                  std::vector<std::string>{PermissionIdentifier(mainnet)}),
              EqualsMojo(mainnet->account_id));
}

TEST_F(PolkadotDappUtilsUnitTest, PreferredAccount_UnknownAllowedAccount) {
  CreateWallet();
  ASSERT_TRUE(AddAccount());

  EXPECT_FALSE(GetPolkadotPreferredDappAccount(
      keyring_service(), std::vector<std::string>{"not-an-account"}));
}

TEST_F(PolkadotDappUtilsUnitTest, MakeInjectedAccount) {
  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);

  auto injected = MakePolkadotInjectedAccount(*account);
  ASSERT_TRUE(injected);
  EXPECT_EQ(injected->address, account->address);
  EXPECT_EQ(injected->name, account->name);
  EXPECT_EQ(injected->type, kSr25519);
  EXPECT_FALSE(injected->genesis_hash);
}

}  // namespace brave_wallet
