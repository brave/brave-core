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
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
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
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"  // IWYU pragma: keep
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
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
  // `EvaluatePermissionsState` and its result enum are private; the fixture is
  // a friend, so this alias is what makes them nameable from the tests.
  using PermissionCheckResult = PolkadotProviderImpl::PermissionCheckResult;

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

  // Accessors ----------------------------------------------------------------

  PolkadotProviderImpl* provider() { return provider_.get(); }

  // The delegate the provider holds for itself.
  MockBraveWalletProviderDelegate* delegate() {
    return static_cast<MockBraveWalletProviderDelegate*>(
        provider_->delegate_.get());
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

  // Reaches through friendship so each branch can be asserted on directly,
  // rather than inferred from the `Enable` result it happens to produce.
  PermissionCheckResult EvaluatePermissionsState(
      std::vector<std::string>& allowed_accounts) {
    return provider_->EvaluatePermissionsState(allowed_accounts);
  }

  PermissionCheckResult EvaluatePermissionsState() {
    std::vector<std::string> ignored;
    return EvaluatePermissionsState(ignored);
  }

  bool HasParkedRequest() const {
    return !provider_->pending_request_permissions_callback_.is_null();
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

// EvaluatePermissionsState ---------------------------------------------------
// One test per branch, in the order the checks run.

TEST_F(PolkadotProviderImplUnitTest, EvaluatePermissionsState_TabInactive) {
  CreateWallet();
  AddAccount();
  ON_CALL(*delegate(), IsTabVisible()).WillByDefault(testing::Return(false));

  EXPECT_EQ(EvaluatePermissionsState(), PermissionCheckResult::kTabInactive);
}

TEST_F(PolkadotProviderImplUnitTest, EvaluatePermissionsState_DeniedGlobally) {
  CreateWallet();
  AddAccount();
  ON_CALL(*delegate(), IsPermissionDenied(mojom::CoinType::DOT))
      .WillByDefault(testing::Return(true));

  EXPECT_EQ(EvaluatePermissionsState(), PermissionCheckResult::kDeniedGlobally);
}

TEST_F(PolkadotProviderImplUnitTest,
       EvaluatePermissionsState_WalletNotCreated) {
  EXPECT_EQ(EvaluatePermissionsState(),
            PermissionCheckResult::kWalletNotCreated);
}

TEST_F(PolkadotProviderImplUnitTest, EvaluatePermissionsState_NoAccounts) {
  // Wallet creation only makes default ETH and SOL accounts.
  CreateWallet();

  EXPECT_EQ(EvaluatePermissionsState(), PermissionCheckResult::kNoAccounts);
}

TEST_F(PolkadotProviderImplUnitTest,
       EvaluatePermissionsState_TestnetAccountIsNotADappAccount) {
  // Testnet and mainnet keyrings may each hold their own number of accounts but
  // because of our key derivation routines, testnet and mainnet accounts have
  // the same keypairs. To this end, letting users select a testnet account for
  // a dApp can have unintended consequences and a false sense of security, so
  // only mainnet accounts are allowed to be selected.

  CreateWallet();
  ASSERT_TRUE(AddAccount(mojom::KeyringId::kPolkadotTestnet));

  EXPECT_EQ(EvaluatePermissionsState(), PermissionCheckResult::kNoAccounts);
}

TEST_F(PolkadotProviderImplUnitTest,
       EvaluatePermissionsState_ImportedAccountIsADappAccount) {
  CreateWallet();
  auto imported = AddAccount(mojom::KeyringId::kPolkadotImport);
  ASSERT_TRUE(imported);
  SetAllowedAccounts({PermissionIdentifier(imported)});

  EXPECT_EQ(EvaluatePermissionsState(),
            PermissionCheckResult::kHasAllowedAccounts);
}

TEST_F(PolkadotProviderImplUnitTest, EvaluatePermissionsState_WalletLocked) {
  CreateWallet();
  ASSERT_TRUE(AddAccount());
  keyring_service()->Lock();

  EXPECT_EQ(EvaluatePermissionsState(), PermissionCheckResult::kWalletLocked);
}

TEST_F(PolkadotProviderImplUnitTest,
       EvaluatePermissionsState_GetAllowedAccountsFailed) {
  CreateWallet();
  ASSERT_TRUE(AddAccount());
  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(testing::Return(std::nullopt));

  EXPECT_EQ(EvaluatePermissionsState(),
            PermissionCheckResult::kGetAllowedAccountsFailed);
}

TEST_F(PolkadotProviderImplUnitTest,
       EvaluatePermissionsState_NeedsPermissionRequest) {
  CreateWallet();
  ASSERT_TRUE(AddAccount());
  SetAllowedAccounts({});

  EXPECT_EQ(EvaluatePermissionsState(),
            PermissionCheckResult::kNeedsPermissionRequest);
}

TEST_F(PolkadotProviderImplUnitTest,
       EvaluatePermissionsState_HasAllowedAccounts) {
  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);
  SetAllowedAccounts({PermissionIdentifier(account)});

  std::vector<std::string> allowed_accounts;
  EXPECT_EQ(EvaluatePermissionsState(allowed_accounts),
            PermissionCheckResult::kHasAllowedAccounts);
  EXPECT_THAT(allowed_accounts,
              testing::ElementsAre(PermissionIdentifier(account)));
}

TEST_F(PolkadotProviderImplUnitTest,
       EvaluatePermissionsState_OffersEveryDappAccountAsCandidate) {
  // Want to make sure we present all mainnet accounts when doing permission
  // checking.

  CreateWallet();
  auto first = AddAccount(mojom::KeyringId::kPolkadotMainnet, 0);
  auto second = AddAccount(mojom::KeyringId::kPolkadotMainnet, 1);
  auto imported = AddAccount(mojom::KeyringId::kPolkadotImport, 0);
  auto testnet = AddAccount(mojom::KeyringId::kPolkadotTestnet, 0);
  auto imported_testnet =
      AddAccount(mojom::KeyringId::kPolkadotImportTestnet, 0);
  ASSERT_TRUE(first);
  ASSERT_TRUE(second);
  ASSERT_TRUE(imported);
  ASSERT_TRUE(testnet);
  ASSERT_TRUE(imported_testnet);

  EXPECT_CALL(*delegate(),
              GetAllowedAccounts(
                  mojom::CoinType::DOT,
                  testing::UnorderedElementsAre(
                      PermissionIdentifier(first), PermissionIdentifier(second),
                      PermissionIdentifier(imported))))
      .WillOnce(testing::Return(std::vector<std::string>()));

  EXPECT_EQ(EvaluatePermissionsState(),
            PermissionCheckResult::kNeedsPermissionRequest);
}

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
      future;
  provider()->Enable(future.GetCallback());

  EXPECT_TRUE(future.Get<0>());
  EXPECT_FALSE(future.Get<1>());
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
      future;
  provider()->Enable(future.GetCallback());

  EXPECT_TRUE(future.Get<0>());
  EXPECT_FALSE(future.Get<1>());
}

TEST_F(PolkadotProviderImplUnitTest, Enable_GrantedRemoteServesTheAccount) {
  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);
  SetAllowedAccounts({PermissionIdentifier(account)});

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      enable_future;
  provider()->Enable(enable_future.GetCallback());

  auto [pending_api, enable_error] = enable_future.Take();
  ASSERT_TRUE(pending_api);
  ASSERT_FALSE(enable_error);

  // Our PolkadotApiImpl is expected to double-check permissions before doing
  // anything like GetAccounts(). Our Impl has its own
  // BraveWalletProviderDelegate we mock.
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
  EXPECT_EQ(accounts->at(0)->address, account->address);
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

TEST_F(PolkadotProviderImplUnitTest, Enable_PermissionRequestDenied) {
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

TEST_F(PolkadotProviderImplUnitTest, Enable_WalletLocked_ParksTheRequest) {
  CreateWallet();
  ASSERT_TRUE(AddAccount());
  keyring_service()->Lock();

  EXPECT_CALL(*delegate(), ShowPanel(url::Origin::Create(GURL(kTestOrigin))))
      .Times(1);

  base::MockCallback<PolkadotProviderImpl::EnableCallback> callback;
  EXPECT_CALL(callback, Run(_, _)).Times(0);
  provider()->Enable(callback.Get());

  EXPECT_TRUE(HasParkedRequest());
}

TEST_F(PolkadotProviderImplUnitTest,
       Enable_WalletLocked_SecondRequestRejected) {
  CreateWallet();
  ASSERT_TRUE(AddAccount());
  keyring_service()->Lock();

  base::MockCallback<PolkadotProviderImpl::EnableCallback> first_callback;
  EXPECT_CALL(first_callback, Run(_, _)).Times(0);
  provider()->Enable(first_callback.Get());

  TestFuture<mojo::PendingRemote<mojom::PolkadotApi>,
             mojom::PolkadotProviderErrorBundlePtr>
      second;
  provider()->Enable(second.GetCallback());

  EXPECT_FALSE(second.Get<0>());
  ASSERT_TRUE(second.Get<1>());
  EXPECT_EQ(second.Get<1>()->code, mojom::PolkadotProviderError::kUnknown);

  // The first request is still waiting.
  EXPECT_TRUE(HasParkedRequest());
}

TEST_F(PolkadotProviderImplUnitTest, Enable_WalletLocked_ResolvesAfterUnlock) {
  CreateWallet();
  auto account = AddAccount();
  ASSERT_TRUE(account);
  SetAllowedAccounts({PermissionIdentifier(account)});
  keyring_service()->Lock();

  base::RunLoop run_loop;
  base::MockCallback<PolkadotProviderImpl::EnableCallback> callback;
  EXPECT_CALL(callback,
              Run(_, EqualsMojo(mojom::PolkadotProviderErrorBundlePtr())))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));

  provider()->Enable(callback.Get());
  ASSERT_TRUE(HasParkedRequest());

  UnlockWallet();
  run_loop.Run();

  EXPECT_FALSE(HasParkedRequest());
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

  base::RunLoop run_loop;
  base::MockCallback<PolkadotProviderImpl::EnableCallback> callback;
  EXPECT_CALL(callback, Run(_, _))
      .WillOnce(base::test::RunOnceClosure(run_loop.QuitClosure()));

  provider()->Enable(callback.Get());
  ASSERT_TRUE(HasParkedRequest());

  UnlockWallet();
  run_loop.Run();
}

}  // namespace brave_wallet
