/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_api_impl.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"  // IWYU pragma: keep
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

using base::test::TestFuture;
using testing::_;

namespace brave_wallet {

namespace {

constexpr char kSr25519[] = "sr25519";

class MockBraveWalletProviderDelegate : public BraveWalletProviderDelegate {
 public:
  MockBraveWalletProviderDelegate() = default;
  ~MockBraveWalletProviderDelegate() override {}

  MOCK_METHOD0(IsTabVisible, bool());
  MOCK_METHOD1(ShowPanel, void(const url::Origin&));
  MOCK_METHOD0(ShowWalletBackup, void());
  MOCK_METHOD0(UnlockWallet, void());
  MOCK_METHOD0(WalletInteractionDetected, void());
  MOCK_METHOD2(ShowAccountCreation,
               void(mojom::CoinType type, const url::Origin& origin));
  MOCK_METHOD4(RequestPermissions,
               void(mojom::CoinType type,
                    const std::vector<std::string>& accounts,
                    const url::Origin& origin,
                    RequestPermissionsCallback));
  MOCK_METHOD2(IsAccountAllowed,
               bool(mojom::CoinType type, const std::string& account));
  MOCK_METHOD2(GetAllowedAccounts,
               std::optional<std::vector<std::string>>(
                   mojom::CoinType type,
                   const std::vector<std::string>& accounts));
  MOCK_METHOD1(IsPermissionDenied, bool(mojom::CoinType type));
  MOCK_METHOD1(AddSolanaConnectedAccount, void(const std::string& account));
  MOCK_METHOD1(RemoveSolanaConnectedAccount, void(const std::string& account));
  MOCK_METHOD1(IsSolanaAccountConnected, bool(const std::string& account));
};

}  // namespace

class PolkadotApiImplUnitTest : public testing::Test {
 public:
  PolkadotApiImplUnitTest() {
    feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletPolkadotFeature,
          {{"polkadot_dapp_support", "true"}}}},
        {});
  }

  ~PolkadotApiImplUnitTest() override = default;

  void SetUp() override {
    RegisterLocalStatePrefs(local_state_.registry());
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());

    brave_wallet_service_ = std::make_unique<BraveWalletService>(
        url_loader_factory_.GetSafeWeakWrapper(),
        TestBraveWalletServiceDelegate::Create(), &prefs_, &local_state_);
  }

  void TearDown() override {
    delegate_ = nullptr;
    api_.reset();
  }

  // Used to create an instance of the PolkadotApiImpl which always grants
  // permission for the provided account.
  void CreateApi(const mojom::AccountIdPtr& granted_account) {
    auto delegate =
        std::make_unique<testing::NiceMock<MockBraveWalletProviderDelegate>>();
    delegate_ = delegate.get();
    ON_CALL(*delegate_,
            IsAccountAllowed(mojom::CoinType::DOT,
                             GetAccountPermissionIdentifier(granted_account)))
        .WillByDefault(testing::Return(true));

    api_ = std::make_unique<PolkadotApiImpl>(
        *brave_wallet_service_, std::move(delegate), granted_account.Clone());
  }

  void CreateWallet() {
    AccountUtils(keyring_service())
        .CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
  }

  mojom::AccountInfoPtr AddAccount() {
    return AccountUtils(keyring_service()).EnsureDotAccount(0);
  }

  mojom::AccountInfoPtr AddImportedAccount() {
    return AccountUtils(keyring_service()).EnsureDotImportAccount(0);
  }

  // Only for the imported accounts.
  void RemoveAccount(const mojom::AccountIdPtr& account_id) {
    TestFuture<bool> future;
    keyring_service()->RemoveAccount(account_id.Clone(), kTestWalletPassword,
                                     future.GetCallback());
    ASSERT_TRUE(future.Get());
  }

  auto GetAccounts(bool any_type = false) {
    TestFuture<std::optional<std::vector<mojom::PolkadotInjectedAccountPtr>>,
               mojom::PolkadotProviderErrorBundlePtr>
        future;
    api_->GetAccounts(any_type, future.GetCallback());
    return future.Take();
  }

  MockBraveWalletProviderDelegate* delegate() { return delegate_; }

  KeyringService* keyring_service() {
    return brave_wallet_service_->keyring_service();
  }

 private:
  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_;

  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<BraveWalletService> brave_wallet_service_;

  raw_ptr<MockBraveWalletProviderDelegate> delegate_ = nullptr;
  std::unique_ptr<PolkadotApiImpl> api_;
};

TEST_F(PolkadotApiImplUnitTest, GetAccounts_ReturnsTheGrantedAccount) {
  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);
  CreateApi(account->account_id);

  auto [accounts, error] = GetAccounts();

  ASSERT_FALSE(error);
  ASSERT_TRUE(accounts);
  ASSERT_EQ(accounts->size(), 1u);
  EXPECT_THAT(accounts->at(0),
              EqualsMojo(mojom::PolkadotInjectedAccount::New(
                  account->address, /*genesis_hash=*/std::nullopt,
                  account->name, kSr25519)));
}

TEST_F(PolkadotApiImplUnitTest, GetAccounts_OnlyEverServesTheGrantedAccount) {
  // Prove that a dApp gets the one account it was granted, never the rest of
  // the wallet.

  CreateWallet();
  auto granted = AddAccount();
  ASSERT_TRUE(granted);
  ASSERT_TRUE(AccountUtils(keyring_service()).EnsureDotAccount(1));
  ASSERT_TRUE(AddImportedAccount());
  CreateApi(granted->account_id);

  auto [accounts, error] = GetAccounts();

  ASSERT_FALSE(error);
  ASSERT_TRUE(accounts);
  ASSERT_EQ(accounts->size(), 1u);
  EXPECT_THAT(accounts->at(0),
              EqualsMojo(mojom::PolkadotInjectedAccount::New(
                  granted->address, /*genesis_hash=*/std::nullopt,
                  granted->name, kSr25519)));
}

TEST_F(PolkadotApiImplUnitTest, GetAccounts_AnyTypeIsIgnored) {
  // Every Polkadot keypair we hold is sr25519 and can derive further keypairs,
  // so the filter has nothing to filter out.
  // See:
  // https://github.com/polkadot-js/extension/blob/d7c9ce214557e8bd359fac29c4bc38d0e329c1d4/packages/extension-base/src/background/handlers/Tabs.ts#L38

  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);
  CreateApi(account->account_id);

  auto [any_type_accounts, any_type_error] = GetAccounts(/*any_type=*/true);
  auto [accounts, error] = GetAccounts(/*any_type=*/false);

  ASSERT_FALSE(any_type_error);
  ASSERT_FALSE(error);
  ASSERT_TRUE(any_type_accounts);
  ASSERT_TRUE(accounts);
  ASSERT_EQ(any_type_accounts->size(), 1u);
  ASSERT_EQ(accounts->size(), 1u);
  EXPECT_THAT(any_type_accounts->at(0), EqualsMojo(accounts->at(0)));
}

TEST_F(PolkadotApiImplUnitTest, GetAccounts_ImportedAccount) {
  CreateWallet();
  auto imported = AddImportedAccount();
  ASSERT_TRUE(imported);
  CreateApi(imported->account_id);

  auto [accounts, error] = GetAccounts();

  ASSERT_FALSE(error);
  ASSERT_TRUE(accounts);
  ASSERT_EQ(accounts->size(), 1u);
  EXPECT_THAT(accounts->at(0),
              EqualsMojo(mojom::PolkadotInjectedAccount::New(
                  imported->address, /*genesis_hash=*/std::nullopt,
                  imported->name, kSr25519)));
}

TEST_F(PolkadotApiImplUnitTest, GetAccounts_RechecksPermissionOnEveryCall) {
  // Prove that our PolkadotApiImpl will recheck permissions on each call to
  // GetAccounts.

  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);
  CreateApi(account->account_id);

  EXPECT_CALL(
      *delegate(),
      IsAccountAllowed(mojom::CoinType::DOT,
                       GetAccountPermissionIdentifier(account->account_id)))
      .Times(2)
      .WillRepeatedly(testing::Return(true));

  auto [first_accounts, first_error] = GetAccounts();
  auto [second_accounts, second_error] = GetAccounts();

  EXPECT_TRUE(first_accounts);
  EXPECT_TRUE(second_accounts);
}

TEST_F(PolkadotApiImplUnitTest, GetAccounts_PermissionRevoked) {
  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);
  CreateApi(account->account_id);

  ON_CALL(*delegate(), IsAccountAllowed(_, _))
      .WillByDefault(testing::Return(false));

  auto [accounts, error] = GetAccounts();

  EXPECT_FALSE(accounts);
  ASSERT_TRUE(error);
  EXPECT_THAT(error,
              EqualsMojo(mojom::PolkadotProviderErrorBundle::New(
                  mojom::PolkadotProviderError::kUnknown,
                  l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST))));
}

TEST_F(PolkadotApiImplUnitTest, GetAccounts_WalletLocked) {
  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);
  CreateApi(account->account_id);
  keyring_service()->Lock();

  auto [accounts, error] = GetAccounts();

  EXPECT_FALSE(accounts);
  ASSERT_TRUE(error);
  EXPECT_THAT(
      error,
      EqualsMojo(mojom::PolkadotProviderErrorBundle::New(
          mojom::PolkadotProviderError::kUnknown,
          l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR))));
}

TEST_F(PolkadotApiImplUnitTest, GetAccounts_AccountRemovedAfterGrant) {
  // Prove that if a user removes an imported account, we correctly do not
  // return it.

  CreateWallet();
  auto imported = AddImportedAccount();
  ASSERT_TRUE(imported);
  CreateApi(imported->account_id);

  RemoveAccount(imported->account_id);

  auto [accounts, error] = GetAccounts();

  EXPECT_FALSE(accounts);
  ASSERT_TRUE(error);
  EXPECT_THAT(error, EqualsMojo(mojom::PolkadotProviderErrorBundle::New(
                         mojom::PolkadotProviderError::kInternalError,
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR))));
}

}  // namespace brave_wallet
