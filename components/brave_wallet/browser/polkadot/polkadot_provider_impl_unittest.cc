/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_provider_impl.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_dapp_utils.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"  // IWYU pragma: keep
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"
#include "url/origin.h"

using base::test::TestFuture;
using testing::_;

namespace brave_wallet {

namespace {

constexpr char kTestOrigin[] = "https://brave.com";

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

class PolkadotProviderImplUnitTest : public testing::Test {
 public:
  PolkadotProviderImplUnitTest() {
    // Must happen before the KeyringService is constructed: the Polkadot
    // keyrings are only enabled when the feature is, and the dapp permission
    // layer only recognises DOT when `polkadot_dapp_support` is on.
    feature_list_.InitWithFeaturesAndParameters(
        {{features::kBraveWalletPolkadotFeature,
          {{"polkadot_dapp_support", "true"}}}},
        {});
  }

  ~PolkadotProviderImplUnitTest() override = default;

  void SetUp() override {
    RegisterLocalStatePrefs(local_state_.registry());
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());

    brave_wallet_service_ = std::make_unique<BraveWalletService>(
        url_loader_factory_.GetSafeWeakWrapper(),
        TestBraveWalletServiceDelegate::Create(), &prefs_, &local_state_);

    provider_ = std::make_unique<PolkadotProviderImpl>(
        *brave_wallet_service_,
        base::BindLambdaForTesting(
            [this]() -> std::unique_ptr<BraveWalletProviderDelegate> {
              auto delegate = std::make_unique<
                  testing::NiceMock<MockBraveWalletProviderDelegate>>();
              delegates_.push_back(delegate.get());
              return delegate;
            }),
        url::Origin::Create(GURL(kTestOrigin)));

    // A NiceMock reports the tab as hidden by default, which short-circuits
    // every permission check. Tests that care override this.
    ON_CALL(*delegate(), IsTabVisible()).WillByDefault(testing::Return(true));
  }

  void TearDown() override {
    // `delegates_` does not own anything; drop the references before the
    // provider frees the mocks they point at.
    delegates_.clear();
    provider_.reset();
  }

  // Test helpers -------------------------------------------------------------

  void CreateWallet() {
    AccountUtils(keyring_service())
        .CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
  }

  mojom::AccountInfoPtr AddAccount(
      mojom::KeyringId keyring_id = mojom::KeyringId::kPolkadotMainnet,
      uint32_t index = 0) {
    return AccountUtils(keyring_service()).EnsureAccount(keyring_id, index);
  }

  void UnlockWallet() {
    TestFuture<bool> future;
    keyring_service()->Unlock(kTestWalletPassword, future.GetCallback());
    ASSERT_TRUE(future.Get());
  }

  // Makes the permission layer report `accounts` as already granted, so
  // `Enable` takes the kHasAllowedAccounts path without prompting.
  void SetAllowedAccounts(std::vector<std::string> accounts) {
    ON_CALL(*delegate(), GetAllowedAccounts(_, _))
        .WillByDefault([accounts = std::move(accounts)](
                           mojom::CoinType coin,
                           const std::vector<std::string>& candidates) {
          EXPECT_EQ(coin, mojom::CoinType::DOT);
          return accounts;
        });
  }

  // Makes the connect prompt resolve with `granted`.
  void SetPermissionRequestResult(mojom::RequestPermissionsError error,
                                  std::vector<std::string> granted) {
    ON_CALL(*delegate(), RequestPermissions(_, _, _, _))
        .WillByDefault(
            [error, granted = std::move(granted)](
                mojom::CoinType coin, const std::vector<std::string>& accounts,
                const url::Origin& origin,
                MockBraveWalletProviderDelegate::RequestPermissionsCallback
                    callback) {
              EXPECT_EQ(coin, mojom::CoinType::DOT);
              EXPECT_EQ(origin, url::Origin::Create(GURL(kTestOrigin)));
              std::move(callback).Run(error, granted);
            });
  }

  std::string PermissionIdentifier(const mojom::AccountInfoPtr& account) {
    return GetAccountPermissionIdentifier(account->account_id);
  }

  PolkadotProviderImpl* provider() { return provider_.get(); }

  // This is assigned in the constructor of our unit test fixture, so we know
  // front() _is_ the delegate used by the PolkadotProviderImpl.
  MockBraveWalletProviderDelegate* delegate() {
    CHECK(!delegates_.empty());
    return delegates_.front();
  }

  // The delegate handed to the `PolkadotApiImpl` a successful `Enable` built.
  // Only meaningful once `Enable` has resolved.
  MockBraveWalletProviderDelegate* api_delegate() {
    CHECK_GT(delegates_.size(), 1u);
    return delegates_.back();
  }

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

  std::vector<raw_ptr<MockBraveWalletProviderDelegate>> delegates_;
  std::unique_ptr<PolkadotProviderImpl> provider_;
};

TEST_F(PolkadotProviderImplUnitTest, Enable_AlreadyPermitted) {
  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);

  // Pre-seeding the allowed accounts puts us into a pre-approved state, so we
  // don't need to request permissions again.
  SetAllowedAccounts({PermissionIdentifier(account)});

  EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);
  EXPECT_CALL(*delegate(), RequestPermissions(_, _, _, _)).Times(0);

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      enable_future;
  provider()->Enable(enable_future.GetCallback());

  auto [pending_api, enable_error] = enable_future.Take();
  ASSERT_TRUE(pending_api);
  ASSERT_FALSE(enable_error);

  ON_CALL(*api_delegate(),
          IsAccountAllowed(mojom::CoinType::DOT, PermissionIdentifier(account)))
      .WillByDefault(testing::Return(true));

  mojo::Remote<mojom::PolkadotApi> api(std::move(pending_api));
  TestFuture<std::optional<std::vector<mojom::PolkadotInjectedAccountPtr>>,
             mojom::PolkadotProviderErrorBundlePtr>
      accounts_future;
  api->GetAccounts(/*any_type=*/false, accounts_future.GetCallback());

  auto [accounts, accounts_error] = accounts_future.Take();
  ASSERT_FALSE(accounts_error);
  ASSERT_TRUE(accounts);
  ASSERT_EQ(accounts->size(), 1u);
  EXPECT_THAT(
      accounts->at(0),
      EqualsMojo(mojom::PolkadotInjectedAccount::New(
          account->address, /*genesis_hash=*/std::nullopt,
          kPolkadotObfuscatedAccountName, kPolkadotSr25519KeypairType)));
}

TEST_F(PolkadotProviderImplUnitTest, Enable_PermissionRequestGranted) {
  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);
  SetAllowedAccounts({});
  SetPermissionRequestResult(mojom::RequestPermissionsError::kNone,
                             {PermissionIdentifier(account)});

  EXPECT_CALL(*delegate(), RequestPermissions(_, _, _, _)).Times(1);

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      enable_future;
  provider()->Enable(enable_future.GetCallback());

  auto [pending_api, enable_error] = enable_future.Take();
  ASSERT_TRUE(pending_api);
  ASSERT_FALSE(enable_error);

  ON_CALL(*api_delegate(),
          IsAccountAllowed(mojom::CoinType::DOT, PermissionIdentifier(account)))
      .WillByDefault(testing::Return(true));

  mojo::Remote<mojom::PolkadotApi> api(std::move(pending_api));
  TestFuture<std::optional<std::vector<mojom::PolkadotInjectedAccountPtr>>,
             mojom::PolkadotProviderErrorBundlePtr>
      accounts_future;
  api->GetAccounts(/*any_type=*/false, accounts_future.GetCallback());

  auto [accounts, accounts_error] = accounts_future.Take();
  ASSERT_FALSE(accounts_error);
  ASSERT_TRUE(accounts);
  ASSERT_EQ(accounts->size(), 1u);
  EXPECT_THAT(
      accounts->at(0),
      EqualsMojo(mojom::PolkadotInjectedAccount::New(
          account->address, /*genesis_hash=*/std::nullopt,
          kPolkadotObfuscatedAccountName, kPolkadotSr25519KeypairType)));
}

TEST_F(PolkadotProviderImplUnitTest, Enable_ImportedAccountIsADappAccount) {
  CreateWallet();
  auto imported = AddAccount(mojom::KeyringId::kPolkadotImport);
  ASSERT_TRUE(imported);
  SetAllowedAccounts({PermissionIdentifier(imported)});

  EXPECT_CALL(*delegate(), RequestPermissions(_, _, _, _)).Times(0);

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      enable_future;
  provider()->Enable(enable_future.GetCallback());

  auto [pending_api, enable_error] = enable_future.Take();
  ASSERT_TRUE(pending_api);
  ASSERT_FALSE(enable_error);

  ON_CALL(*api_delegate(), IsAccountAllowed(mojom::CoinType::DOT,
                                            PermissionIdentifier(imported)))
      .WillByDefault(testing::Return(true));

  mojo::Remote<mojom::PolkadotApi> api(std::move(pending_api));
  TestFuture<std::optional<std::vector<mojom::PolkadotInjectedAccountPtr>>,
             mojom::PolkadotProviderErrorBundlePtr>
      accounts_future;
  api->GetAccounts(/*any_type=*/false, accounts_future.GetCallback());

  auto [accounts, accounts_error] = accounts_future.Take();
  ASSERT_FALSE(accounts_error);
  ASSERT_TRUE(accounts);
  ASSERT_EQ(accounts->size(), 1u);
  EXPECT_THAT(
      accounts->at(0),
      EqualsMojo(mojom::PolkadotInjectedAccount::New(
          imported->address, /*genesis_hash=*/std::nullopt,
          kPolkadotObfuscatedAccountName, kPolkadotSr25519KeypairType)));
}

TEST_F(PolkadotProviderImplUnitTest, Enable_TabInactive) {
  CreateWallet();
  ASSERT_TRUE(AddAccount());
  ON_CALL(*delegate(), IsTabVisible()).WillByDefault(testing::Return(false));

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      future;
  provider()->Enable(future.GetCallback());

  EXPECT_FALSE(future.Get<0>());
  ASSERT_TRUE(future.Get<1>());
  EXPECT_EQ(future.Get<1>()->code, mojom::PolkadotProviderError::kUnknown);
}

TEST_F(PolkadotProviderImplUnitTest, Enable_DeniedGlobally) {
  CreateWallet();
  ASSERT_TRUE(AddAccount());
  ON_CALL(*delegate(), IsPermissionDenied(mojom::CoinType::DOT))
      .WillByDefault(testing::Return(true));

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      future;
  provider()->Enable(future.GetCallback());

  EXPECT_FALSE(future.Get<0>());
  ASSERT_TRUE(future.Get<1>());
  EXPECT_EQ(future.Get<1>()->code, mojom::PolkadotProviderError::kUnknown);
}

TEST_F(PolkadotProviderImplUnitTest, Enable_NoWallet) {
  EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      future;
  provider()->Enable(future.GetCallback());

  EXPECT_FALSE(future.Get<0>());
  EXPECT_TRUE(future.Get<1>());
}

TEST_F(PolkadotProviderImplUnitTest, Enable_GetAllowedAccountsFailed) {
  CreateWallet();
  ASSERT_TRUE(AddAccount());
  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(testing::Return(std::nullopt));

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      future;
  provider()->Enable(future.GetCallback());

  EXPECT_FALSE(future.Get<0>());
  ASSERT_TRUE(future.Get<1>());
  EXPECT_EQ(future.Get<1>()->code,
            mojom::PolkadotProviderError::kInternalError);
}

TEST_F(PolkadotProviderImplUnitTest, Enable_PermissionRequestDeclined) {
  // Declining the prompt arrives as `kNone` with nothing granted rather than as
  // an error.

  CreateWallet();
  ASSERT_TRUE(AddAccount());
  SetAllowedAccounts({});
  SetPermissionRequestResult(mojom::RequestPermissionsError::kNone, {});

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      future;
  provider()->Enable(future.GetCallback());

  auto [pending_api, error] = future.Take();

  EXPECT_FALSE(pending_api);
  EXPECT_THAT(error,
              EqualsMojo(mojom::PolkadotProviderErrorBundle::New(
                  mojom::PolkadotProviderError::kUnknown,
                  l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST))));
}

TEST_F(PolkadotProviderImplUnitTest, Enable_PermissionRequestInternalError) {
  CreateWallet();
  ASSERT_TRUE(AddAccount());
  SetAllowedAccounts({});
  SetPermissionRequestResult(mojom::RequestPermissionsError::kInternal, {});

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      future;
  provider()->Enable(future.GetCallback());

  EXPECT_FALSE(future.Get<0>());
  ASSERT_TRUE(future.Get<1>());
  EXPECT_EQ(future.Get<1>()->code,
            mojom::PolkadotProviderError::kInternalError);
}

TEST_F(PolkadotProviderImplUnitTest, Enable_PermissionRequestInProgress) {
  CreateWallet();
  ASSERT_TRUE(AddAccount());
  SetAllowedAccounts({});
  SetPermissionRequestResult(mojom::RequestPermissionsError::kRequestInProgress,
                             {});

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      future;
  provider()->Enable(future.GetCallback());

  EXPECT_FALSE(future.Get<0>());
  ASSERT_TRUE(future.Get<1>());
  EXPECT_EQ(future.Get<1>()->code, mojom::PolkadotProviderError::kUnknown);
}

TEST_F(PolkadotProviderImplUnitTest, Enable_AllowedAccountIsUnknown) {
  // Prove that no matter what our allowed accounts are, we still check against
  // what's currently in the KeyringService.

  CreateWallet();
  ASSERT_TRUE(AddAccount());
  SetAllowedAccounts({"not-an-account-we-hold"});

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      future;
  provider()->Enable(future.GetCallback());

  EXPECT_FALSE(future.Get<0>());
  ASSERT_TRUE(future.Get<1>());
  EXPECT_EQ(future.Get<1>()->code, mojom::PolkadotProviderError::kUnknown);
}

TEST_F(PolkadotProviderImplUnitTest, Enable_NoAccounts_ShowsAccountCreation) {
  CreateWallet();

  EXPECT_CALL(*delegate(),
              ShowAccountCreation(mojom::CoinType::DOT,
                                  url::Origin::Create(GURL(kTestOrigin))))
      .Times(1);

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      future;
  provider()->Enable(future.GetCallback());

  EXPECT_FALSE(future.Get<0>());
  EXPECT_TRUE(future.Get<1>());
}

TEST_F(PolkadotProviderImplUnitTest,
       Enable_NoAccounts_ShowsAccountCreationOnlyOnce) {
  // Prove that a malicious dApp can't trick our code into looping something
  // like the account creation screen.

  CreateWallet();

  EXPECT_CALL(*delegate(), ShowAccountCreation(_, _)).Times(1);

  for (int i = 0; i < 3; ++i) {
    TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
               mojom::PolkadotProviderErrorBundlePtr>
        future;
    provider()->Enable(future.GetCallback());
    EXPECT_FALSE(future.Get<0>());
    EXPECT_TRUE(future.Get<1>());
  }
}

TEST_F(PolkadotProviderImplUnitTest, Enable_TestnetAccountIsNotADappAccount) {
  CreateWallet();
  ASSERT_TRUE(AddAccount(mojom::KeyringId::kPolkadotTestnet));

  EXPECT_CALL(*delegate(), GetAllowedAccounts(_, _)).Times(0);
  EXPECT_CALL(*delegate(),
              ShowAccountCreation(mojom::CoinType::DOT,
                                  url::Origin::Create(GURL(kTestOrigin))))
      .Times(1);

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      future;
  provider()->Enable(future.GetCallback());

  auto [pending_api, error] = future.Take();

  EXPECT_FALSE(pending_api);
  EXPECT_THAT(error,
              EqualsMojo(mojom::PolkadotProviderErrorBundle::New(
                  mojom::PolkadotProviderError::kUnknown,
                  l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST))));
}

TEST_F(PolkadotProviderImplUnitTest, Enable_OffersEveryDappAccountAsCandidate) {
  CreateWallet();
  auto first = AddAccount(mojom::KeyringId::kPolkadotMainnet, 0);
  auto second = AddAccount(mojom::KeyringId::kPolkadotMainnet, 1);
  auto imported = AddAccount(mojom::KeyringId::kPolkadotImport, 0);
  ASSERT_TRUE(AddAccount(mojom::KeyringId::kPolkadotTestnet));
  ASSERT_TRUE(first);
  ASSERT_TRUE(second);
  ASSERT_TRUE(imported);
  ASSERT_TRUE(AddAccount(mojom::KeyringId::kPolkadotTestnet, 0));
  ASSERT_TRUE(AddAccount(mojom::KeyringId::kPolkadotImportTestnet, 0));

  const std::string first_id = PermissionIdentifier(first);
  const std::vector<std::string> dapp_accounts = {
      first_id, PermissionIdentifier(second), PermissionIdentifier(imported)};

  EXPECT_CALL(
      *delegate(),
      GetAllowedAccounts(mojom::CoinType::DOT,
                         testing::UnorderedElementsAreArray(dapp_accounts)))
      .WillOnce(testing::Return(std::vector<std::string>()));

  EXPECT_CALL(*delegate(),
              RequestPermissions(
                  mojom::CoinType::DOT,
                  testing::UnorderedElementsAreArray(dapp_accounts), _, _))
      .WillOnce(
          [first_id](mojom::CoinType, const std::vector<std::string>&,
                     const url::Origin&,
                     MockBraveWalletProviderDelegate::RequestPermissionsCallback
                         callback) {
            std::move(callback).Run(mojom::RequestPermissionsError::kNone,
                                    std::vector<std::string>{first_id});
          });

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      future;
  provider()->Enable(future.GetCallback());

  auto [pending_api, error] = future.Take();
  EXPECT_TRUE(pending_api);
  EXPECT_FALSE(error);
}

TEST_F(PolkadotProviderImplUnitTest, Enable_WalletLocked_ParksTheRequest) {
  CreateWallet();
  ASSERT_TRUE(AddAccount());
  keyring_service()->Lock();

  EXPECT_CALL(*delegate(), ShowPanel(url::Origin::Create(GURL(kTestOrigin))))
      .Times(1);

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      future;
  provider()->Enable(future.GetCallback());

  EXPECT_FALSE(future.IsReady());
}

TEST_F(PolkadotProviderImplUnitTest,
       Enable_WalletLocked_SecondRequestRejected) {
  CreateWallet();
  ASSERT_TRUE(AddAccount());
  keyring_service()->Lock();

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      first;
  provider()->Enable(first.GetCallback());

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      second;
  provider()->Enable(second.GetCallback());

  auto [polkadot_api, error] = second.Take();

  EXPECT_FALSE(polkadot_api);
  ASSERT_TRUE(error);
  EXPECT_THAT(error,
              EqualsMojo(mojom::PolkadotProviderErrorBundle::New(
                  mojom::PolkadotProviderError::kUnknown,
                  l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST))));

  // The first request is still waiting.
  EXPECT_FALSE(first.IsReady());
}

TEST_F(PolkadotProviderImplUnitTest, Enable_WalletLocked_ResolvesAfterUnlock) {
  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);
  SetAllowedAccounts({PermissionIdentifier(account)});
  keyring_service()->Lock();

  base::test::TestFuture<::mojo::PendingRemote<mojom::PolkadotApi>,
                         mojom::PolkadotProviderErrorBundlePtr>
      future;

  provider()->Enable(future.GetCallback());
  ASSERT_FALSE(future.IsReady());

  UnlockWallet();
  auto [polkadot_api, error] = future.Take();

  EXPECT_FALSE(error);
}

TEST_F(PolkadotProviderImplUnitTest,
       Enable_WalletLocked_ReevaluatedAfterUnlock) {
  // Prove that unlocking the wallet with a pending request still goes through
  // the same permission evaluation routines indirectly via the call to the
  // BraveWalletProvider's GetAllowedAccounts call.

  CreateWallet();
  ASSERT_TRUE(AddAccount());
  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(testing::Return(std::nullopt));
  keyring_service()->Lock();

  base::test::TestFuture<::mojo::PendingRemote<mojom::PolkadotApi>,
                         mojom::PolkadotProviderErrorBundlePtr>
      future;

  provider()->Enable(future.GetCallback());
  ASSERT_FALSE(future.IsReady());

  UnlockWallet();

  auto [polkadot_api, error] = future.Take();

  EXPECT_THAT(error, EqualsMojo(mojom::PolkadotProviderErrorBundle::New(
                         mojom::PolkadotProviderError::kInternalError,
                         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR))));
}

}  // namespace brave_wallet
