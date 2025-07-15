/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_provider_impl.h"

#include <memory>
#include <optional>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_profile.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/browser_test_utils.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

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
  CardanoProviderImplUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}
  ~CardanoProviderImplUnitTest() override = default;

  void SetUp() override {
    RegisterLocalStatePrefs(local_state_.registry());
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    brave_wallet_service_ = std::make_unique<BraveWalletService>(
        shared_url_loader_factory_, TestBraveWalletServiceDelegate::Create(),
        &prefs_, &local_state_);
    provider_ = std::make_unique<CardanoProviderImpl>(
        *brave_wallet_service_,
        std::make_unique<testing::NiceMock<MockBraveWalletProviderDelegate>>());
  }

  void CreateWallet() {
    AccountUtils(keyring_service())
        .CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
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

  KeyringService* keyring_service() {
    return brave_wallet_service_->keyring_service();
  }

 private:
  base::test::ScopedFeatureList feature_list_{
      features::kBraveWalletCardanoFeature};

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<BraveWalletService> brave_wallet_service_;

  std::unique_ptr<CardanoProviderImpl> provider_;
};

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
  base::RunLoop main_run_loop;
  EXPECT_CALL(first_callback,
              Run(EqualsMojo(mojom::CardanoProviderErrorBundlePtr())))
      .WillOnce(base::test::RunOnceClosure(main_run_loop.QuitClosure()));
  provider()->Enable(first_callback.Get());

  {
    // Request will be rejected because it is still waiting for wallet unlock.
    base::test::TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
    provider()->Enable(future.GetCallback());
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

  base::test::TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
  provider()->Enable(future.GetCallback());
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

  base::test::TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
  provider()->Enable(future.GetCallback());
  auto error = future.Take();
  EXPECT_TRUE(error);
}

TEST_F(CardanoProviderImplUnitTest, MethodReturnsError_WhenNoPermission) {
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

  EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(0);

  {
    base::test::TestFuture<int32_t, mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetNetworkId(future.GetCallback());
    auto [networkId, error] = future.Take();
    EXPECT_EQ(0, networkId);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::vector<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUsedAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_TRUE(addrs.empty());
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::vector<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUnusedAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_TRUE(addrs.empty());
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::string&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetChangeAddress(future.GetCallback());
    auto [addr, error] = future.Take();
    EXPECT_EQ(addr, "");
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::vector<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetRewardAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_TRUE(addrs.empty());
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::string&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;

    provider()->GetBalance(future.GetCallback());
    auto [balance, error] = future.Take();
    EXPECT_TRUE(balance.empty());
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUtxos(std::nullopt, nullptr, future.GetCallback());
    auto [utxos, error] = future.Take();
    EXPECT_FALSE(utxos);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::string&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->SignTx("", false, future.GetCallback());
    auto [tx, error] = future.Take();
    EXPECT_EQ("", tx);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::string&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->SubmitTx("", future.GetCallback());
    auto [tx_hash, error] = future.Take();
    EXPECT_EQ("", tx_hash);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<mojom::CardanoProviderSignatureResultPtr,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->SignData("", "", future.GetCallback());
    auto [data, error] = future.Take();
    EXPECT_FALSE(data);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetCollateral("", future.GetCallback());
    auto [result, error] = future.Take();
    EXPECT_FALSE(result);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }
}

TEST_F(CardanoProviderImplUnitTest, MethodReturnsSuccess_WhenHasPermission) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();
  EXPECT_TRUE(added_account);

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            EXPECT_EQ(coin, mojom::CoinType::ADA);
            EXPECT_EQ(accounts.size(), 1u);
            EXPECT_EQ(accounts[0], added_account->account_id->unique_key);
            return std::vector<std::string>(
                {added_account->account_id->unique_key});
          });
  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<int32_t, mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetNetworkId(future.GetCallback());
    auto [networkId, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::vector<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUsedAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::vector<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUnusedAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::string&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetChangeAddress(future.GetCallback());
    auto [addr, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::vector<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetRewardAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::string&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;

    provider()->GetBalance(future.GetCallback());
    auto [balance, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUtxos(std::nullopt, nullptr, future.GetCallback());
    auto [utxos, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::string&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->SignTx("", false, future.GetCallback());
    auto [tx, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::string&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->SubmitTx("", future.GetCallback());
    auto [tx_hash, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<mojom::CardanoProviderSignatureResultPtr,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->SignData("", "", future.GetCallback());
    auto [data, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetCollateral("", future.GetCallback());
    auto [result, error] = future.Take();
    EXPECT_FALSE(error);
  }
}

}  // namespace brave_wallet
