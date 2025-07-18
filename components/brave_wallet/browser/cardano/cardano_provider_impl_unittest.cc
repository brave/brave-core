/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_provider_impl.h"

#include <memory>
#include <optional>
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
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

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
        *brave_wallet_service_,
        std::make_unique<testing::NiceMock<MockBraveWalletProviderDelegate>>());
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
  base::test::ScopedFeatureList feature_list_{
      features::kBraveWalletCardanoFeature};

  base::ScopedTempDir temp_dir_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<BraveWalletService> brave_wallet_service_;

  std::unique_ptr<CardanoProviderImpl> provider_;
  base::test::TaskEnvironment task_environment_;
};

TEST_F(CardanoProviderImplUnitTest, SignData_Approved) {
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

  auto address = keyring_service()->GetCardanoAddress(
      added_account->account_id,
      mojom::CardanoKeyId::New(mojom::CardanoKeyRole::kExternal, 0));

  SignMessageRequestWaiter waiter(brave_wallet_service());

  TestFuture<base::Value::Dict, mojom::CardanoProviderErrorBundlePtr> future;

  provider()->SignData(address->address_string, base::HexEncode("message"),
                       future.GetCallback());

  waiter.WaitAndProcess(true);

  auto& signature = future.Get<0>();
  auto& error = future.Get<1>();

  base::Value::Dict expected_signature;
  expected_signature.Set(
      "key",
      "a50101025839010fdc780023d8be7c9ff3a6bdc0d8d3b263bd0cc12448c40948efbf42e5"
      "57890352095f1cf6fd2b7d1a28e3c3cb029f48cf34ff890a28d176032720062158207ea0"
      "9a34aebb13c9841c71397b1cabfec5ddf950405293dee496cac2f437480a");
  expected_signature.Set(
      "signature",
      "845882a30127045839010fdc780023d8be7c9ff3a6bdc0d8d3b263bd0cc12448c40948ef"
      "bf42e557890352095f1cf6fd2b7d1a28e3c3cb029f48cf34ff890a28d176676164647265"
      "73735839010fdc780023d8be7c9ff3a6bdc0d8d3b263bd0cc12448c40948efbf42e55789"
      "0352095f1cf6fd2b7d1a28e3c3cb029f48cf34ff890a28d176a166686173686564f4476d"
      "657373616765584082b878be1769641643040d851379d8dacf398377133edd1a5022751b"
      "d24bc5d769aa720d83faf653953865b3c104c766da9273f164e831cdf76bc1370c4f5d0"
      "c");

  EXPECT_EQ(signature, expected_signature);
  EXPECT_FALSE(error);
}

TEST_F(CardanoProviderImplUnitTest, SignData_Rejected) {
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

  auto address = keyring_service()->GetCardanoAddress(
      added_account->account_id,
      mojom::CardanoKeyId::New(mojom::CardanoKeyRole::kExternal, 0));

  SignMessageRequestWaiter waiter(brave_wallet_service());

  TestFuture<base::Value::Dict, mojom::CardanoProviderErrorBundlePtr> future;

  provider()->SignData(address->address_string, base::HexEncode("message"),
                       future.GetCallback());

  waiter.WaitAndProcess(false);

  auto& signature = future.Get<0>();
  auto& error = future.Get<1>();

  base::Value::Dict expected_signature;
  EXPECT_EQ(signature, expected_signature);
  EXPECT_EQ(error, mojom::CardanoProviderErrorBundle::New(3, "", nullptr));
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
  base::RunLoop main_run_loop;
  EXPECT_CALL(first_callback,
              Run(EqualsMojo(mojom::CardanoProviderErrorBundlePtr())))
      .WillOnce(base::test::RunOnceClosure(main_run_loop.QuitClosure()));
  provider()->Enable(first_callback.Get());

  {
    // Request will be rejected because it is still waiting for wallet unlock.
    TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
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

  TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
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

  TestFuture<mojom::CardanoProviderErrorBundlePtr> future;
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
    TestFuture<int32_t, mojom::CardanoProviderErrorBundlePtr> future;
    provider()->GetNetworkId(future.GetCallback());
    auto [networkId, error] = future.Take();
    EXPECT_EQ(0, networkId);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    TestFuture<const std::vector<std::string>&,
               mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUsedAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_TRUE(addrs.empty());
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    TestFuture<const std::vector<std::string>&,
               mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUnusedAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_TRUE(addrs.empty());
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    TestFuture<const std::string&, mojom::CardanoProviderErrorBundlePtr> future;
    provider()->GetChangeAddress(future.GetCallback());
    auto [addr, error] = future.Take();
    EXPECT_EQ(addr, "");
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    TestFuture<const std::vector<std::string>&,
               mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetRewardAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_TRUE(addrs.empty());
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    TestFuture<const std::string&, mojom::CardanoProviderErrorBundlePtr> future;

    provider()->GetBalance(future.GetCallback());
    auto [balance, error] = future.Take();
    EXPECT_TRUE(balance.empty());
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    TestFuture<const std::optional<std::vector<std::string>>&,
               mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUtxos(std::nullopt, nullptr, future.GetCallback());
    auto [utxos, error] = future.Take();
    EXPECT_FALSE(utxos);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    TestFuture<const std::string&, mojom::CardanoProviderErrorBundlePtr> future;
    provider()->SignTx("", false, future.GetCallback());
    auto [tx, error] = future.Take();
    EXPECT_EQ("", tx);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    TestFuture<const std::string&, mojom::CardanoProviderErrorBundlePtr> future;
    provider()->SubmitTx("", future.GetCallback());
    auto [tx_hash, error] = future.Take();
    EXPECT_EQ("", tx_hash);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    TestFuture<base::Value::Dict, mojom::CardanoProviderErrorBundlePtr> future;
    provider()->SignData("", "", future.GetCallback());
    auto [data, error] = future.Take();
    EXPECT_EQ(data, base::Value::Dict());
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    TestFuture<const std::optional<std::vector<std::string>>&,
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

    TestFuture<int32_t, mojom::CardanoProviderErrorBundlePtr> future;
    provider()->GetNetworkId(future.GetCallback());
    auto [networkId, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    TestFuture<const std::vector<std::string>&,
               mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUsedAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    TestFuture<const std::vector<std::string>&,
               mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUnusedAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    TestFuture<const std::string&, mojom::CardanoProviderErrorBundlePtr> future;
    provider()->GetChangeAddress(future.GetCallback());
    auto [addr, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    TestFuture<const std::vector<std::string>&,
               mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetRewardAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    TestFuture<const std::string&, mojom::CardanoProviderErrorBundlePtr> future;

    provider()->GetBalance(future.GetCallback());
    auto [balance, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    TestFuture<const std::optional<std::vector<std::string>>&,
               mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUtxos(std::nullopt, nullptr, future.GetCallback());
    auto [utxos, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    TestFuture<const std::string&, mojom::CardanoProviderErrorBundlePtr> future;
    provider()->SignTx("", false, future.GetCallback());
    auto [tx, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    TestFuture<const std::string&, mojom::CardanoProviderErrorBundlePtr> future;
    provider()->SubmitTx("", future.GetCallback());
    auto [tx_hash, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    TestFuture<base::Value::Dict, mojom::CardanoProviderErrorBundlePtr> future;
    provider()->SignData("", "", future.GetCallback());
    auto [data, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    TestFuture<const std::optional<std::vector<std::string>>&,
               mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetCollateral("", future.GetCallback());
    auto [result, error] = future.Take();
    EXPECT_FALSE(error);
  }
}

}  // namespace brave_wallet
