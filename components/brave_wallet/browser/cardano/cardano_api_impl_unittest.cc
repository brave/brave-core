/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_api_impl.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_cip30_serializer.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
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

std::vector<std::pair<CardanoAddress, cardano_rpc::UnspentOutput>>
UtxosToVector(
    const std::map<std::string,
                   std::vector<cardano_rpc::blockfrost_api::UnspentOutput>>&
        map) {
  std::vector<std::pair<CardanoAddress, cardano_rpc::UnspentOutput>> result;
  for (const auto& by_addr : map) {
    for (const auto& utxo : by_addr.second) {
      result.push_back(
          {CardanoAddress::FromString(by_addr.first).value(),
           cardano_rpc::UnspentOutput::FromBlockfrostApiValue(utxo.Clone())
               .value()});
    }
  }
  return result;
}

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

class CardanoApiImplTest : public testing::Test {
 public:
  CardanoApiImplTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}
  ~CardanoApiImplTest() override = default;

  void SetUp() override {
    RegisterLocalStatePrefs(local_state_.registry());
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
    brave_wallet_service_ = std::make_unique<BraveWalletService>(
        shared_url_loader_factory_, TestBraveWalletServiceDelegate::Create(),
        &prefs_, &local_state_);
    provider_ = std::make_unique<CardanoApiImpl>(
        *brave_wallet_service_,
        std::make_unique<testing::NiceMock<MockBraveWalletProviderDelegate>>(),
        MakeIndexBasedAccountId(mojom::CoinType::ADA,
                                mojom::KeyringId::kCardanoMainnet,
                                mojom::AccountKind::kDerived, 0));
    cardano_test_rpc_server_ = std::make_unique<CardanoTestRpcServer>(
        *(brave_wallet_service_->GetCardanoWalletService()));
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

  CardanoApiImpl* provider() { return provider_.get(); }

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

  CardanoTestRpcServer* test_rpc_service() {
    return cardano_test_rpc_server_.get();
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
  std::unique_ptr<CardanoTestRpcServer> cardano_test_rpc_server_;

  std::unique_ptr<CardanoApiImpl> provider_;
};

TEST_F(CardanoApiImplTest, GetNetworkId) {
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

  TestFuture<int32_t, mojom::CardanoProviderErrorBundlePtr> future;

  provider()->GetNetworkId(future.GetCallback());

  auto& network_id = future.Get<0>();
  auto& error = future.Get<1>();

  EXPECT_EQ(network_id, 1);
  EXPECT_FALSE(error);
}

TEST_F(CardanoApiImplTest, GetUsedAddresses) {
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

  TestFuture<const std::optional<std::vector<std::string>>&,
             mojom::CardanoProviderErrorBundlePtr>
      future;

  provider()->GetUsedAddresses(future.GetCallback());

  auto& addresses = future.Get<0>();
  auto& error = future.Get<1>();

  EXPECT_EQ(addresses,
            std::vector<std::string>{
                "010fdc780023d8be7c9ff3a6bdc0d8d3b263bd0cc12448c40948efbf42e557"
                "890352095f1cf6fd2b7d1a28e3c3cb029f48cf34ff890a28d176"});
  EXPECT_FALSE(error);
}

TEST_F(CardanoApiImplTest, GetUnusedAddresses) {
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

  TestFuture<const std::optional<std::vector<std::string>>&,
             mojom::CardanoProviderErrorBundlePtr>
      future;

  provider()->GetUnusedAddresses(future.GetCallback());

  auto& addresses = future.Get<0>();
  auto& error = future.Get<1>();

  EXPECT_EQ(addresses, std::vector<std::string>{});
  EXPECT_FALSE(error);
}

TEST_F(CardanoApiImplTest, GetChangeAddress) {
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

  TestFuture<const std::optional<std::string>&,
             mojom::CardanoProviderErrorBundlePtr>
      future;

  provider()->GetChangeAddress(future.GetCallback());

  auto& address = future.Get<0>();
  auto& error = future.Get<1>();

  EXPECT_EQ(address,
            "010fdc780023d8be7c9ff3a6bdc0d8d3b263bd0cc12448c40948efbf42e5578903"
            "52095f1cf6fd2b7d1a28e3c3cb029f48cf34ff890a28d176");
  EXPECT_FALSE(error);
}

TEST_F(CardanoApiImplTest, SignData_Approved) {
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

  TestFuture<std::optional<base::Value::Dict>,
             mojom::CardanoProviderErrorBundlePtr>
      future;

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

TEST_F(CardanoApiImplTest, SignData_Rejected) {
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

  TestFuture<std::optional<base::Value::Dict>,
             mojom::CardanoProviderErrorBundlePtr>
      future;

  provider()->SignData(address->address_string, base::HexEncode("message"),
                       future.GetCallback());

  waiter.WaitAndProcess(false);

  auto& signature = future.Get<0>();
  auto& error = future.Get<1>();

  EXPECT_EQ(signature, std::nullopt);
  EXPECT_EQ(error,
            mojom::CardanoProviderErrorBundle::New(
                3, l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST),
                nullptr));
}

TEST_F(CardanoApiImplTest, MethodReturnsError_WhenNoPermission) {
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
    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUsedAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_FALSE(addrs);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUnusedAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_FALSE(addrs);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::optional<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetChangeAddress(future.GetCallback());
    auto [addr, error] = future.Take();
    EXPECT_FALSE(addr);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetRewardAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_FALSE(addrs);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::optional<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;

    provider()->GetBalance(future.GetCallback());
    auto [balance, error] = future.Take();
    EXPECT_FALSE(balance);
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
    base::test::TestFuture<const std::optional<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->SignTx("", false, future.GetCallback());
    auto [tx, error] = future.Take();
    EXPECT_FALSE(tx);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::optional<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->SubmitTx("", future.GetCallback());
    auto [tx_hash, error] = future.Take();
    EXPECT_FALSE(tx_hash);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<std::optional<base::DictValue>,
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

TEST_F(CardanoApiImplTest, MethodReturnsError_WhenAccountChanged) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();

  auto new_account = MakeIndexBasedAccountId(mojom::CoinType::ADA,
                                             mojom::KeyringId::kCardanoMainnet,
                                             mojom::AccountKind::kDerived, 1);

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            EXPECT_EQ(coin, mojom::CoinType::ADA);
            EXPECT_EQ(accounts.size(), 1u);
            EXPECT_EQ(accounts[0], added_account->account_id->unique_key);
            return std::vector<std::string>(
                {GetAccountPermissionIdentifier(new_account)});
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
    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUsedAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_FALSE(addrs);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUnusedAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_FALSE(addrs);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::optional<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetChangeAddress(future.GetCallback());
    auto [addr, error] = future.Take();
    EXPECT_FALSE(addr);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetRewardAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_FALSE(addrs);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::optional<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;

    provider()->GetBalance(future.GetCallback());
    auto [balance, error] = future.Take();
    EXPECT_FALSE(balance);
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
    base::test::TestFuture<const std::optional<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->SignTx("", false, future.GetCallback());
    auto [tx, error] = future.Take();
    EXPECT_FALSE(tx);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<const std::optional<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->SubmitTx("", future.GetCallback());
    auto [tx_hash, error] = future.Take();
    EXPECT_FALSE(tx_hash);
    EXPECT_EQ(error->code, -3);
    EXPECT_FALSE(error->pagination_error_payload);
  }

  {
    base::test::TestFuture<std::optional<base::DictValue>,
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

TEST_F(CardanoApiImplTest, MethodReturnsSuccess_WhenHasPermission) {
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

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUsedAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUnusedAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetChangeAddress(future.GetCallback());
    auto [addr, error] = future.Take();
    EXPECT_FALSE(error);
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetRewardAddresses(future.GetCallback());
    auto [addrs, error] = future.Take();
    EXPECT_EQ(error->error_message, "Not implemented");
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::string>&,
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

    base::test::TestFuture<const std::optional<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->SignTx("", false, future.GetCallback());
    auto [tx, error] = future.Take();
    EXPECT_EQ(error->error_message, "Not implemented");
  }

  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetCollateral("", future.GetCallback());
    auto [result, error] = future.Take();
    EXPECT_EQ(error->error_message, "Not implemented");
  }
}

TEST_F(CardanoApiImplTest, GetBalance) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();
  EXPECT_TRUE(added_account);

  test_rpc_service()->AddUtxo(brave_wallet_service()
                                  ->GetCardanoWalletService()
                                  ->GetChangeAddress(added_account->account_id)
                                  ->address_string,
                              100000);

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

    base::test::TestFuture<const std::optional<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetBalance(future.GetCallback());
    auto [balance, error] = future.Take();
    EXPECT_FALSE(error);
    EXPECT_EQ(balance.value(),
              CardanoCip30Serializer::SerializeAmount(100000u));
  }
}

TEST_F(CardanoApiImplTest, GetBalance_Error) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();
  EXPECT_TRUE(added_account);

  test_rpc_service()->set_fail_address_utxo_request(true);

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

    base::test::TestFuture<const std::optional<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetBalance(future.GetCallback());
    auto [balance, error] = future.Take();
    EXPECT_TRUE(error);
  }
}

TEST_F(CardanoApiImplTest, GetUtxos_Error) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();
  EXPECT_TRUE(added_account);

  test_rpc_service()->set_fail_address_utxo_request(true);

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

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUtxos(std::nullopt, nullptr, future.GetCallback());
    auto [utxos, error] = future.Take();
    EXPECT_TRUE(error);
  }
}

TEST_F(CardanoApiImplTest, GetUtxos) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();
  EXPECT_TRUE(added_account);

  test_rpc_service()->AddUtxo(brave_wallet_service()
                                  ->GetCardanoWalletService()
                                  ->GetChangeAddress(added_account->account_id)
                                  ->address_string,
                              100000);
  test_rpc_service()->AddUtxo(brave_wallet_service()
                                  ->GetCardanoWalletService()
                                  ->GetChangeAddress(added_account->account_id)
                                  ->address_string,
                              200000);
  test_rpc_service()->AddUtxo(brave_wallet_service()
                                  ->GetCardanoWalletService()
                                  ->GetChangeAddress(added_account->account_id)
                                  ->address_string,
                              300000);
  test_rpc_service()->AddUtxo(brave_wallet_service()
                                  ->GetCardanoWalletService()
                                  ->GetChangeAddress(added_account->account_id)
                                  ->address_string,
                              400000);

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            EXPECT_EQ(coin, mojom::CoinType::ADA);
            EXPECT_EQ(accounts.size(), 1u);
            EXPECT_EQ(accounts[0], added_account->account_id->unique_key);
            return std::vector<std::string>(
                {added_account->account_id->unique_key});
          });

  // No args
  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUtxos(std::nullopt, nullptr, future.GetCallback());
    auto [utxos, error] = future.Take();

    EXPECT_EQ(utxos.value(),
              CardanoCip30Serializer::SerializeUtxos(
                  UtxosToVector(test_rpc_service()->GetUtxos())));

    EXPECT_FALSE(error);
  }

  // Amount limit
  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUtxos(CardanoCip30Serializer::SerializeAmount(600000u),
                         nullptr, future.GetCallback());
    auto [utxos, error] = future.Take();
    auto utxos_as_vec = UtxosToVector(test_rpc_service()->GetUtxos());
    EXPECT_TRUE(utxos);
    EXPECT_EQ(utxos.value(),
              CardanoCip30Serializer::SerializeUtxos(
                  std::vector(utxos_as_vec.begin(), utxos_as_vec.begin() + 3)));
    EXPECT_FALSE(error);
  }

  // Amount exceeds
  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUtxos(CardanoCip30Serializer::SerializeAmount(10000000u),
                         nullptr, future.GetCallback());
    auto [utxos, error] = future.Take();
    EXPECT_FALSE(utxos);
    EXPECT_FALSE(error);
  }

  // Amount limit with pagination
  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUtxos(CardanoCip30Serializer::SerializeAmount(600000u),
                         mojom::CardanoProviderPagination::New(1, 2),
                         future.GetCallback());
    auto [utxos, error] = future.Take();
    auto utxos_as_vec = UtxosToVector(test_rpc_service()->GetUtxos());
    EXPECT_TRUE(utxos);
    EXPECT_EQ(utxos.value(),
              CardanoCip30Serializer::SerializeUtxos(std::vector(
                  utxos_as_vec.begin() + 2, utxos_as_vec.begin() + 3)));
    EXPECT_FALSE(error);
  }

  // No amount limit with pagination
  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUtxos(std::nullopt,
                         mojom::CardanoProviderPagination::New(3, 1),
                         future.GetCallback());
    auto [utxos, error] = future.Take();
    auto utxos_as_vec = UtxosToVector(test_rpc_service()->GetUtxos());
    EXPECT_TRUE(utxos);
    EXPECT_EQ(utxos.value(),
              CardanoCip30Serializer::SerializeUtxos(std::vector(
                  utxos_as_vec.begin() + 3, utxos_as_vec.begin() + 4)));
    EXPECT_FALSE(error);
  }

  // No amount limit with pagination
  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUtxos(std::nullopt,
                         mojom::CardanoProviderPagination::New(3, 1),
                         future.GetCallback());
    auto [utxos, error] = future.Take();
    auto utxos_as_vec = UtxosToVector(test_rpc_service()->GetUtxos());
    EXPECT_TRUE(utxos);
    EXPECT_EQ(utxos.value(),
              CardanoCip30Serializer::SerializeUtxos(std::vector(
                  utxos_as_vec.begin() + 3, utxos_as_vec.begin() + 4)));
    EXPECT_FALSE(error);
  }

  // Paginate error
  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUtxos(std::nullopt,
                         mojom::CardanoProviderPagination::New(10, 1),
                         future.GetCallback());
    auto [utxos, error] = future.Take();
    EXPECT_FALSE(utxos);
    EXPECT_TRUE(error);
    EXPECT_EQ(error->pagination_error_payload->payload, 4);
  }

  // Paginate error - limit is zero
  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUtxos(std::nullopt,
                         mojom::CardanoProviderPagination::New(10, 0),
                         future.GetCallback());
    auto [utxos, error] = future.Take();
    EXPECT_FALSE(utxos);
    EXPECT_TRUE(error);
  }

  // Paginate error - arguments less than 0
  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUtxos(std::nullopt,
                         mojom::CardanoProviderPagination::New(-1, -1),
                         future.GetCallback());
    auto [utxos, error] = future.Take();
    EXPECT_FALSE(utxos);
    EXPECT_TRUE(error);
  }
}

TEST_F(CardanoApiImplTest, GetUtxos_NumericOverflow) {
  CreateWallet();
  UnlockWallet();
  auto added_account = AddAccount();
  EXPECT_TRUE(added_account);

  test_rpc_service()->AddUtxo(brave_wallet_service()
                                  ->GetCardanoWalletService()
                                  ->GetChangeAddress(added_account->account_id)
                                  ->address_string,
                              UINT64_MAX - 100);
  test_rpc_service()->AddUtxo(brave_wallet_service()
                                  ->GetCardanoWalletService()
                                  ->GetChangeAddress(added_account->account_id)
                                  ->address_string,
                              UINT64_MAX - 200);

  ON_CALL(*delegate(), GetAllowedAccounts(_, _))
      .WillByDefault(
          [&](mojom::CoinType coin, const std::vector<std::string>& accounts) {
            EXPECT_EQ(coin, mojom::CoinType::ADA);
            EXPECT_EQ(accounts.size(), 1u);
            EXPECT_EQ(accounts[0], added_account->account_id->unique_key);
            return std::vector<std::string>(
                {added_account->account_id->unique_key});
          });

  // Amount exceeds
  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUtxos(
        CardanoCip30Serializer::SerializeAmount(UINT64_MAX - 50), nullptr,
        future.GetCallback());
    auto [utxos, error] = future.Take();
    EXPECT_FALSE(utxos);
    EXPECT_TRUE(error);
  }
}

TEST_F(CardanoApiImplTest, SubmitTx_Fails) {
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

  test_rpc_service()->FailNextTransactionSubmission();
  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->SubmitTx("aaaa", future.GetCallback());
    auto [tx_hash, error] = future.Take();
    EXPECT_FALSE(tx_hash);
    EXPECT_TRUE(error);
    EXPECT_EQ(error->code, 2);
  }
}

TEST_F(CardanoApiImplTest, SubmitTx) {
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

    base::test::TestFuture<const std::optional<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->SubmitTx("aaaa", future.GetCallback());
    auto [tx_hash, error] = future.Take();
    EXPECT_EQ(kMockCardanoTxid, *tx_hash);
    EXPECT_FALSE(error);
  }
}

TEST_F(CardanoApiImplTest, SubmitTx_FailsNotHex) {
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

    base::test::TestFuture<const std::optional<std::string>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->SubmitTx("rrrr", future.GetCallback());
    auto [tx_hash, error] = future.Take();
    EXPECT_FALSE(tx_hash);
    EXPECT_TRUE(error);
    EXPECT_EQ(error->code, 2);
  }
}

}  // namespace brave_wallet
