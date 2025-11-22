/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_provider_impl.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

using base::test::TestFuture;
using testing::_;

namespace brave_wallet {

namespace {

class MockBraveWalletProviderDelegate : public BraveWalletProviderDelegate {
 public:
  MockBraveWalletProviderDelegate() = default;
  ~MockBraveWalletProviderDelegate() override {}

  MOCK_METHOD0(IsTabVisible, bool());
  MOCK_METHOD0(ShowPanel, void());
  MOCK_METHOD0(ShowWalletBackup, void());
  MOCK_METHOD0(UnlockWallet, void());
  MOCK_METHOD0(WalletInteractionDetected, void());
  MOCK_METHOD0(ShowWalletOnboarding, void());
  MOCK_METHOD1(ShowAccountCreation, void(mojom::CoinType type));
  MOCK_CONST_METHOD0(GetOrigin, url::Origin());
  MOCK_METHOD3(RequestPermissions,
               void(mojom::CoinType type,
                    const std::vector<std::string>& accounts,
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

class CardanoProviderImplUnitTest : public testing::Test {
 public:
  CardanoProviderImplUnitTest() = default;
  ~CardanoProviderImplUnitTest() override = default;

  void SetUp() override {
    RegisterLocalStatePrefs(local_state_.registry());
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    brave_wallet_service_ = std::make_unique<BraveWalletService>(
        url_loader_factory_.GetSafeWeakWrapper(),
        TestBraveWalletServiceDelegate::Create(), &prefs_, &local_state_);
    provider_ = std::make_unique<CardanoProviderImpl>(
        *brave_wallet_service_, base::BindLambdaForTesting([]() {
          std::unique_ptr<BraveWalletProviderDelegate> result =
              std::make_unique<
                  testing::NiceMock<MockBraveWalletProviderDelegate>>();
          return result;
        }));
  }

  void CreateWallet() {
    AccountUtils(keyring_service())
        .CreateWallet(kMnemonicAbandonAbandon, kTestWalletPassword);
  }

  mojom::AccountInfoPtr AddAccount() {
    return keyring_service()->AddAccountSync(
        mojom::CoinType::ADA, mojom::KeyringId::kCardanoMainnet, "New Account");
  }

  void UnlockWallet() {
    base::RunLoop run_loop;
    keyring_service()->Unlock(
        "brave", base::BindLambdaForTesting([&run_loop](bool success) {
          ASSERT_TRUE(success);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  CardanoProviderImpl* provider() { return provider_.get(); }

  MockBraveWalletProviderDelegate* delegate() {
    return static_cast<MockBraveWalletProviderDelegate*>(
        provider()->delegate());
  }

  BraveWalletService* brave_wallet_service() {
    return brave_wallet_service_.get();
  }

  KeyringService* keyring_service() {
    return brave_wallet_service_->keyring_service();
  }

  CardanoWalletService* cardano_wallet_service() {
    return brave_wallet_service_->GetCardanoWalletService();
  }

 private:
  base::test::TaskEnvironment task_environment_;
  base::test::ScopedFeatureList feature_list_{
      features::kBraveWalletCardanoFeature};

  base::ScopedTempDir temp_dir_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<BraveWalletService> brave_wallet_service_;

  std::unique_ptr<CardanoProviderImpl> provider_;
};

TEST_F(CardanoProviderImplUnitTest, Enable_PermissionApproved) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            EXPECT_EQ(coin, mojom::CoinType::ADA);
            EXPECT_EQ(accounts.size(), 1u);
            EXPECT_EQ(accounts[0], added_account->account_id->unique_key);
            return std::vector<std::string>(
                {added_account->account_id->unique_key});
          });

  ON_CALL(*delegate(), RequestPermissions(_, _, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts,
              MockBraveWalletProviderDelegate::RequestPermissionsCallback
                  callback) {
            EXPECT_EQ(coin, mojom::CoinType::ADA);
            EXPECT_EQ(accounts.size(), 1u);
            EXPECT_EQ(accounts[0], added_account->account_id->unique_key);
            std::move(callback).Run(
                mojom::RequestPermissionsError::kNone,
                std::vector<std::string>(
                    {added_account->account_id->unique_key}));
          });

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
  mojo::Remote<mojom::CardanoApi> api;
  provider()->Enable(api.BindNewPipeAndPassReceiver(), future.GetCallback());
  auto error = future.Take();
  EXPECT_FALSE(error);
}

TEST_F(CardanoProviderImplUnitTest, IsEnabled) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            EXPECT_EQ(coin, mojom::CoinType::ADA);
            EXPECT_EQ(accounts.size(), 1u);
            EXPECT_EQ(accounts[0], added_account->account_id->unique_key);
            return std::vector<std::string>(
                {added_account->account_id->unique_key});
          });

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  TestFuture<bool> future;
  provider()->IsEnabled(future.GetCallback());
  EXPECT_TRUE(future.Take());
}

TEST_F(CardanoProviderImplUnitTest, IsEnabled_AllowedAccountsFailed) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            return std::nullopt;
          });

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  TestFuture<bool> future;
  provider()->IsEnabled(future.GetCallback());
  EXPECT_FALSE(future.Take());
}

TEST_F(CardanoProviderImplUnitTest, IsEnabled_TabNotVisible) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            EXPECT_EQ(coin, mojom::CoinType::ADA);
            EXPECT_EQ(accounts.size(), 1u);
            EXPECT_EQ(accounts[0], added_account->account_id->unique_key);
            return std::vector<std::string>(
                {added_account->account_id->unique_key});
          });

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return false; });

  TestFuture<bool> future;
  provider()->IsEnabled(future.GetCallback());
  EXPECT_FALSE(future.Take());
}

TEST_F(CardanoProviderImplUnitTest, IsEnabled_WalletNotCreated) {
  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  TestFuture<bool> future;
  provider()->IsEnabled(future.GetCallback());
  EXPECT_FALSE(future.Take());
}

TEST_F(CardanoProviderImplUnitTest, IsEnabled_AccountNotCreated) {
  CreateWallet();
  UnlockWallet();

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  TestFuture<bool> future;
  provider()->IsEnabled(future.GetCallback());
  EXPECT_FALSE(future.Take());
}

TEST_F(CardanoProviderImplUnitTest, IsEnabled_NoAllowedAccounts) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            EXPECT_EQ(coin, mojom::CoinType::ADA);
            EXPECT_EQ(accounts.size(), 1u);
            EXPECT_EQ(accounts[0], added_account->account_id->unique_key);
            return std::vector<std::string>();
          });

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  TestFuture<bool> future;
  provider()->IsEnabled(future.GetCallback());
  EXPECT_FALSE(future.Take());
}

TEST_F(CardanoProviderImplUnitTest, IsEnabled_WalletLocked) {
  CreateWallet();
  keyring_service()->Lock();
  auto added_account = AddAccount();

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            EXPECT_EQ(coin, mojom::CoinType::ADA);
            EXPECT_EQ(accounts.size(), 1u);
            EXPECT_EQ(accounts[0], added_account->account_id->unique_key);
            return std::vector<std::string>(
                {added_account->account_id->unique_key});
          });

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  TestFuture<bool> future;
  provider()->IsEnabled(future.GetCallback());
  EXPECT_FALSE(future.Take());
}

TEST_F(CardanoProviderImplUnitTest, Enable_OnWalletUnlock_PermissionApproved) {
  CreateWallet();
  auto added_account = AddAccount();

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            EXPECT_EQ(coin, mojom::CoinType::ADA);
            EXPECT_EQ(accounts.size(), 1u);
            EXPECT_EQ(accounts[0], added_account->account_id->unique_key);
            return std::vector<std::string>(
                {added_account->account_id->unique_key});
          });

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  keyring_service()->Lock();

  base::MockCallback<CardanoProviderImpl::EnableCallback> first_callback;
  mojo::Remote<mojom::CardanoApi> api;
  base::RunLoop main_run_loop;
  EXPECT_CALL(first_callback,
              Run(EqualsMojo(mojom::CardanoProviderErrorBundlePtr())))
      .WillOnce(base::test::RunOnceClosure(main_run_loop.QuitClosure()));
  provider()->Enable(api.BindNewPipeAndPassReceiver(), first_callback.Get());

  {
    mojo::Remote<mojom::CardanoApi> api2;
    // Request will be rejected because it is still waiting for wallet unlock.
    TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
    provider()->Enable(api2.BindNewPipeAndPassReceiver(), future.GetCallback());
    auto error = future.Take();
    EXPECT_TRUE(error);
  }

  ON_CALL(*delegate(), RequestPermissions(_, _, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts,
              MockBraveWalletProviderDelegate::RequestPermissionsCallback
                  callback) {
            EXPECT_EQ(coin, mojom::CoinType::ADA);
            EXPECT_EQ(accounts.size(), 1u);
            EXPECT_EQ(accounts[0], added_account->account_id->unique_key);
            std::move(callback).Run(
                mojom::RequestPermissionsError::kNone,
                std::vector<std::string>(
                    {added_account->account_id->unique_key}));
          });

  UnlockWallet();

  main_run_loop.Run();
}

TEST_F(CardanoProviderImplUnitTest, OnBoarding) {
  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });
  EXPECT_CALL(*delegate(), ShowWalletOnboarding()).Times(1);

  mojo::Remote<mojom::CardanoApi> api;
  base::test::TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
  provider()->Enable(api.BindNewPipeAndPassReceiver(), future.GetCallback());

  EXPECT_TRUE(future.Get<0>());
}

TEST_F(CardanoProviderImplUnitTest, AccCreation) {
  CreateWallet();

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });
  EXPECT_CALL(*delegate(),
              ShowAccountCreation(testing::Eq(mojom::CoinType::ADA)))
      .Times(1);

  mojo::Remote<mojom::CardanoApi> api;
  base::test::TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
  provider()->Enable(api.BindNewPipeAndPassReceiver(), future.GetCallback());

  EXPECT_TRUE(future.Get<0>());
}

TEST_F(CardanoProviderImplUnitTest,
       EnableFails_OnWalletUnlock_PermissionDenied) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            EXPECT_EQ(coin, mojom::CoinType::ADA);
            EXPECT_EQ(accounts.size(), 1u);
            EXPECT_EQ(accounts[0], added_account->account_id->unique_key);
            return std::vector<std::string>();
          });

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  ON_CALL(*delegate(), RequestPermissions(_, _, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts,
              MockBraveWalletProviderDelegate::RequestPermissionsCallback
                  callback) {
            EXPECT_EQ(coin, mojom::CoinType::ADA);
            EXPECT_EQ(accounts.size(), 1u);
            EXPECT_EQ(accounts[0], added_account->account_id->unique_key);
            std::move(callback).Run(mojom::RequestPermissionsError::kInternal,
                                    std::vector<std::string>());
          });

  TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
  provider()->Enable(mojo::PendingReceiver<mojom::CardanoApi>(),
                     future.GetCallback());
  auto error = future.Take();
  EXPECT_TRUE(error);
}

TEST_F(CardanoProviderImplUnitTest, EnableFails_OnWalletUnlock_TabNotActive) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            EXPECT_EQ(coin, mojom::CoinType::ADA);
            EXPECT_EQ(accounts.size(), 1u);
            EXPECT_EQ(accounts[0], added_account->account_id->unique_key);
            return std::vector<std::string>();
          });

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return false; });

  TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
  provider()->Enable(mojo::PendingReceiver<mojom::CardanoApi>(),
                     future.GetCallback());
  auto error = future.Take();
  EXPECT_TRUE(error);
}

TEST_F(CardanoProviderImplUnitTest, Enable_GlobalPermissionDenial) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  ON_CALL(*delegate(), IsPermissionDenied(mojom::CoinType::ADA))
      .WillByDefault([&]() { return true; });

  TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
  mojo::Remote<mojom::CardanoApi> api;
  provider()->Enable(api.BindNewPipeAndPassReceiver(), future.GetCallback());

  auto error = future.Take();
  ASSERT_TRUE(error);
  EXPECT_EQ(error->code, -3);  
  EXPECT_EQ(error->error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
}

TEST_F(CardanoProviderImplUnitTest, IsEnabled_GlobalPermissionDenial) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  ON_CALL(*delegate(), IsPermissionDenied(mojom::CoinType::ADA))
      .WillByDefault([&]() { return true; });

  TestFuture<bool> future;
  provider()->IsEnabled(future.GetCallback());
  EXPECT_FALSE(future.Take());
}

TEST_F(CardanoProviderImplUnitTest, Enable_OriginValidation) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();

  const url::Origin expected_origin =
      url::Origin::Create(GURL("https://example.com"));

  EXPECT_CALL(*delegate(), GetOrigin())
      .WillOnce(testing::Return(expected_origin));

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            return std::vector<std::string>(
                {added_account->account_id->unique_key});
          });

  TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
  mojo::Remote<mojom::CardanoApi> api;
  provider()->Enable(api.BindNewPipeAndPassReceiver(), future.GetCallback());

  auto error = future.Take();
  EXPECT_FALSE(error);
}

TEST_F(CardanoProviderImplUnitTest, Enable_UnlockRetainsCapturedOrigin) {
  CreateWallet();
  auto added_account = AddAccount();

  const url::Origin initial_origin =
      url::Origin::Create(GURL("https://initial.com"));

  EXPECT_CALL(*delegate(), GetOrigin())
      .WillOnce(testing::Return(initial_origin));

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            return std::vector<std::string>(
                {added_account->account_id->unique_key});
          });

  keyring_service()->Lock();

  base::MockCallback<CardanoProviderImpl::EnableCallback> callback;
  mojo::Remote<mojom::CardanoApi> api;
  base::RunLoop main_run_loop;
  EXPECT_CALL(callback, Run(EqualsMojo(mojom::CardanoProviderErrorBundlePtr())))
      .WillOnce(base::test::RunOnceClosure(main_run_loop.QuitClosure()));

  provider()->Enable(api.BindNewPipeAndPassReceiver(), callback.Get());

  ON_CALL(*delegate(), RequestPermissions(_, _, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts,
              MockBraveWalletProviderDelegate::RequestPermissionsCallback
                  callback) {
            std::move(callback).Run(
                mojom::RequestPermissionsError::kNone,
                std::vector<std::string>(
                    {added_account->account_id->unique_key}));
          });

  UnlockWallet();
  main_run_loop.Run();
}

TEST_F(CardanoProviderImplUnitTest, Enable_MultipleCallsCreateSeparateAPIs) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            return std::vector<std::string>(
                {added_account->account_id->unique_key});
          });

  TestFuture<mojom::CardanoProviderErrorBundlePtr> future1;
  mojo::Remote<mojom::CardanoApi> api1;
  provider()->Enable(api1.BindNewPipeAndPassReceiver(), future1.GetCallback());

  auto error1 = future1.Take();
  EXPECT_FALSE(error1);
  EXPECT_TRUE(api1.is_bound());

  TestFuture<mojom::CardanoProviderErrorBundlePtr> future2;
  mojo::Remote<mojom::CardanoApi> api2;
  provider()->Enable(api2.BindNewPipeAndPassReceiver(), future2.GetCallback());

  auto error2 = future2.Take();
  EXPECT_FALSE(error2);
  EXPECT_TRUE(api2.is_bound());

  EXPECT_TRUE(api1.is_bound());
  EXPECT_TRUE(api2.is_bound());
}

TEST_F(CardanoProviderImplUnitTest, Enable_RequestInProgressError) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            return std::vector<std::string>();
          });

  ON_CALL(*delegate(), RequestPermissions(_, _, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts,
              MockBraveWalletProviderDelegate::RequestPermissionsCallback
                  callback) {
            std::move(callback).Run(
                mojom::RequestPermissionsError::kRequestInProgress,
                std::nullopt);
          });

  TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
  mojo::Remote<mojom::CardanoApi> api;
  provider()->Enable(api.BindNewPipeAndPassReceiver(), future.GetCallback());

  auto error = future.Take();
  ASSERT_TRUE(error);
  EXPECT_EQ(error->code, -3);  
  EXPECT_EQ(error->error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
}

TEST_F(CardanoProviderImplUnitTest, Enable_InternalError) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            return std::vector<std::string>();
          });

  ON_CALL(*delegate(), RequestPermissions(_, _, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts,
              MockBraveWalletProviderDelegate::RequestPermissionsCallback
                  callback) {
            std::move(callback).Run(mojom::RequestPermissionsError::kInternal,
                                    std::nullopt);
          });

  TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
  mojo::Remote<mojom::CardanoApi> api;
  provider()->Enable(api.BindNewPipeAndPassReceiver(), future.GetCallback());

  auto error = future.Take();
  ASSERT_TRUE(error);
  EXPECT_EQ(error->code, -2);  
  EXPECT_EQ(error->error_message,
            l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
}

TEST_F(CardanoProviderImplUnitTest, Enable_AccountValidationAfterApproval) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();

  bool permission_granted = false;

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            if (permission_granted) {
              return std::vector<std::string>(
                  {added_account->account_id->unique_key});
            }
            return std::vector<std::string>();
          });

  ON_CALL(*delegate(), RequestPermissions(_, _, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts,
              MockBraveWalletProviderDelegate::RequestPermissionsCallback
                  callback) {
            permission_granted = true;  
            std::move(callback).Run(
                mojom::RequestPermissionsError::kNone,
                std::vector<std::string>(
                    {added_account->account_id->unique_key}));
          });

  TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
  mojo::Remote<mojom::CardanoApi> api;
  provider()->Enable(api.BindNewPipeAndPassReceiver(), future.GetCallback());

  auto error = future.Take();
  EXPECT_FALSE(error);
}

TEST_F(CardanoProviderImplUnitTest, Enable_DuplicateUnlockRequestsDropped) {
  CreateWallet();
  auto added_account = AddAccount();
  keyring_service()->Lock();

  ON_CALL(*delegate(), IsTabVisible()).WillByDefault([&]() { return true; });

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            return std::vector<std::string>(
                {added_account->account_id->unique_key});
          });

  TestFuture<mojom::CardanoProviderErrorBundlePtr> future1;
  mojo::Remote<mojom::CardanoApi> api1;
  provider()->Enable(api1.BindNewPipeAndPassReceiver(), future1.GetCallback());

  TestFuture<mojom::CardanoProviderErrorBundlePtr> future2;
  mojo::Remote<mojom::CardanoApi> api2;
  provider()->Enable(api2.BindNewPipeAndPassReceiver(), future2.GetCallback());

  auto error2 = future2.Take();
  ASSERT_TRUE(error2);
  EXPECT_EQ(error2->code, -3);  

  TestFuture<mojom::CardanoProviderErrorBundlePtr> future3;
  mojo::Remote<mojom::CardanoApi> api3;
  provider()->Enable(api3.BindNewPipeAndPassReceiver(), future3.GetCallback());

  auto error3 = future3.Take();
  ASSERT_TRUE(error3);
  EXPECT_EQ(error3->code, -3);  
}

}  // namespace brave_wallet
