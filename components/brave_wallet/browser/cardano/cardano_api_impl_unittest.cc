/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_api_impl.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/containers/span_writer.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_cip30_serializer.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_transaction_serializer.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

using base::test::TestFuture;
using testing::_;

namespace brave_wallet {

namespace {

cardano_rpc::UnspentOutputs UtxosToVector(auto& map) {
  cardano_rpc::UnspentOutputs result;
  for (const auto& by_addr : map) {
    for (const auto& utxo : by_addr.second) {
      result.push_back(
          cardano_rpc::UnspentOutput::FromBlockfrostApiValue(
              *CardanoAddress::FromString(by_addr.first), utxo.Clone())
              .value());
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
    auto delegate =
        std::make_unique<testing::NiceMock<MockBraveWalletProviderDelegate>>();
    ON_CALL(*delegate, GetOrigin).WillByDefault([&]() {
      return url::Origin::Create(GURL("https://brave.com"));
    });
    provider_ = std::make_unique<CardanoApiImpl>(
        *brave_wallet_service_, std::move(delegate),
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

  void SetupUnsignedReferenceTransaction(const mojom::AccountInfoPtr& account,
                                         CardanoTransaction& tx) {
    keyring_service()->UpdateNextUnusedAddressForCardanoAccount(
        account->account_id, 1, 1);
    auto input_address_1 = keyring_service()->GetCardanoAddress(
        account->account_id,
        mojom::CardanoKeyId::New(mojom::CardanoKeyRole::kExternal, 0));

    auto input_address_2 = keyring_service()->GetCardanoAddress(
        account->account_id,
        mojom::CardanoKeyId::New(mojom::CardanoKeyRole::kExternal, 1));

    {
      test_rpc_service()->AddUtxo(
          input_address_1->address_string,
          "a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240add8c71edb925",
          "0", "34451133");

      CardanoTransaction::TxInput input;
      input.utxo_outpoint.txid = test::HexToArray<32>(
          "a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240add8c71edb925");
      input.utxo_outpoint.index = 0;
      input.utxo_value = 34451133;
      tx.AddInput(std::move(input));
    }

    {
      test_rpc_service()->AddUtxo(
          input_address_2->address_string,
          "a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240add8c71edb925",
          "1", "34451133");

      CardanoTransaction::TxInput input;
      input.utxo_outpoint.txid = test::HexToArray<32>(
          "a7b4c1021fa375a4fccb1ac1b3bb01743b3989b5eb732cc6240add8c71edb925");
      input.utxo_outpoint.index = 1;
      input.utxo_value = 34451133;
      tx.AddInput(std::move(input));
    }

    // External
    CardanoTransaction::TxOutput output1;
    output1.address = *CardanoAddress::FromString(
        "addr1q9zwt6rfn2e3mc63hesal6muyg807cwjnkwg3j5azkvmxm0tyqeyc8eu034zzmj4z"
        "53"
        "l7lh5u7z08l0rvp49ht88s5uskl6tsl");
    output1.amount = 10000000;
    tx.AddOutput(std::move(output1));

    CardanoTransaction::TxOutput output2;
    output2.address = *CardanoAddress::FromString(
        "addr1q8s90ehlgwwkq637d3r6qzuxwu6qnprphqadn9pjg2mtcp9hkfmyv4zfhyefvjmpw"
        "w7"
        "f7w9gwem3x6gcm3ulw3kpcgws9sgrhg");
    output2.amount = 24282816;
    output2.type = CardanoTransaction::TxOutputType::kChange;
    tx.AddOutput(std::move(output2));

    // Change
    CardanoTransaction::TxOutput output3;
    output3.address =
        *CardanoAddress::FromString(input_address_1->address_string);
    output3.amount = 24282816;
    output3.type = CardanoTransaction::TxOutputType::kChange;
    tx.AddOutput(std::move(output3));

    tx.set_invalid_after(149770436);
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
  test_rpc_service()->AddUtxo(brave_wallet_service()
                                  ->GetCardanoWalletService()
                                  ->GetChangeAddress(added_account->account_id)
                                  ->address_string,
                              500000);

  auto utxos_as_vec = UtxosToVector(test_rpc_service()->GetUtxos());
  auto utxos_span = base::span(utxos_as_vec);

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
    EXPECT_TRUE(utxos);
    EXPECT_EQ(utxos.value(),
              CardanoCip30Serializer::SerializeUtxos(utxos_span.first(3u)));
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
    EXPECT_TRUE(utxos);
    EXPECT_EQ(utxos.value(), CardanoCip30Serializer::SerializeUtxos(
                                 utxos_span.subspan(2u, 1u)));
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
    EXPECT_TRUE(utxos);
    EXPECT_EQ(utxos.value(), CardanoCip30Serializer::SerializeUtxos(
                                 utxos_span.subspan(3u, 1u)));
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
    EXPECT_TRUE(utxos);
    EXPECT_EQ(utxos.value(), CardanoCip30Serializer::SerializeUtxos(
                                 utxos_span.subspan(3u, 1u)));
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
    EXPECT_EQ(error->pagination_error_payload->payload, 5);
  }

  // Paginate error
  {
    EXPECT_CALL(*delegate(), WalletInteractionDetected()).Times(1);

    base::test::TestFuture<const std::optional<std::vector<std::string>>&,
                           mojom::CardanoProviderErrorBundlePtr>
        future;
    provider()->GetUtxos(std::nullopt,
                         mojom::CardanoProviderPagination::New(3, 3),
                         future.GetCallback());
    auto [utxos, error] = future.Take();
    EXPECT_FALSE(utxos);
    EXPECT_TRUE(error);
    // 5 utxos, 3 page limit, so 2 pages.
    EXPECT_EQ(error->pagination_error_payload->payload, 2);
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

TEST_F(CardanoApiImplTest, SignTx_WrongTransactionFormat) {
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

  TestFuture<const std::optional<std::string>&,
             mojom::CardanoProviderErrorBundlePtr>
      future;

  provider()->SignTx(base::HexEncode("0000"), false, future.GetCallback());

  auto& signed_tx = future.Get<0>();
  auto& error = future.Get<1>();

  EXPECT_FALSE(signed_tx);
  EXPECT_TRUE(error);
}

TEST_F(CardanoApiImplTest, SignTx_Declined) {
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

  CardanoTransaction unsigned_tx;
  SetupUnsignedReferenceTransaction(added_account, unsigned_tx);

  auto unsigned_tx_bytes =
      CardanoTransactionSerializer().SerializeTransaction(unsigned_tx);

  TestFuture<const std::optional<std::string>&,
             mojom::CardanoProviderErrorBundlePtr>
      future;

  SignCardanoTransactionRequestWaiter waiter(brave_wallet_service());

  provider()->SignTx(base::HexEncode(unsigned_tx_bytes), true,
                     future.GetCallback());

  waiter.WaitAndProcess(false);

  auto& signed_tx = future.Get<0>();
  auto& error = future.Get<1>();

  EXPECT_FALSE(signed_tx);
  EXPECT_EQ(error, mojom::CardanoProviderErrorBundle::New(
                       -3, WalletUserRejectedRequestErrorMessage(), nullptr));
}

TEST_F(CardanoApiImplTest, SignTx_DeclinedByPartialSignError) {
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

  CardanoTransaction tx;
  SetupUnsignedReferenceTransaction(added_account, tx);

  // Add an external input.
  CardanoTransaction::TxInput input;
  input.utxo_outpoint.txid.fill(55u);
  input.utxo_outpoint.index = 0;
  input.utxo_value = 34451133;
  tx.AddInput(std::move(input));

  // Add an external witness.
  CardanoTransaction::TxWitness external_witness;
  external_witness.witness_bytes.fill(1);

  auto tx_bytes = CardanoTransactionSerializer().SerializeTransaction(tx);

  {
    auto sign_result =
        brave_wallet_service()->keyring_service()->SignMessageByCardanoKeyring(
            added_account->account_id,
            mojom::CardanoKeyId::New(mojom::CardanoKeyRole::kExternal, 0),
            tx_bytes);

    CardanoTransaction::TxWitness witness;
    auto span_writer = base::SpanWriter(base::span(witness.witness_bytes));
    span_writer.Write(sign_result->pubkey);
    span_writer.Write(sign_result->signature);
    tx.AddWitness(std::move(witness));
  }

  TestFuture<const std::optional<std::string>&,
             mojom::CardanoProviderErrorBundlePtr>
      future;

  provider()->SignTx(base::HexEncode(tx_bytes), false, future.GetCallback());

  auto& signed_tx = future.Get<0>();
  auto& error = future.Get<1>();

  EXPECT_FALSE(signed_tx);
  EXPECT_TRUE(error);
}

TEST_F(CardanoApiImplTest, SignTx) {
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

  CardanoTransaction unsigned_tx;
  SetupUnsignedReferenceTransaction(added_account, unsigned_tx);

  auto unsigned_tx_bytes =
      CardanoTransactionSerializer().SerializeTransaction(unsigned_tx);

  auto tx_hash = CardanoTransactionSerializer().GetTxHash(unsigned_tx);
  std::vector<CardanoSignMessageResult> sign_results;
  sign_results.emplace_back(
      brave_wallet_service()
          ->keyring_service()
          ->SignMessageByCardanoKeyring(
              added_account->account_id,
              mojom::CardanoKeyId::New(mojom::CardanoKeyRole::kExternal, 0),
              tx_hash)
          .value());
  sign_results.emplace_back(
      brave_wallet_service()
          ->keyring_service()
          ->SignMessageByCardanoKeyring(
              added_account->account_id,
              mojom::CardanoKeyId::New(mojom::CardanoKeyRole::kExternal, 1),
              tx_hash)
          .value());

  CardanoTransaction signed_tx = unsigned_tx;
  for (const auto& sign_result : sign_results) {
    CardanoTransaction::TxWitness witness;
    auto span_writer = base::SpanWriter(base::span(witness.witness_bytes));
    span_writer.Write(sign_result.pubkey);
    span_writer.Write(sign_result.signature);
    signed_tx.AddWitness(std::move(witness));
  }
  auto signed_tx_bytes =
      CardanoTransactionSerializer().SerializeTransaction(signed_tx);

  TestFuture<const std::optional<std::string>&,
             mojom::CardanoProviderErrorBundlePtr>
      future;

  SignCardanoTransactionRequestWaiter waiter(brave_wallet_service());
  provider()->SignTx(base::HexEncode(unsigned_tx_bytes), true,
                     future.GetCallback());

  mojom::SignCardanoTransactionRequestPtr request = waiter.WaitAndProcess(true);

  auto addr1 = keyring_service()
                   ->GetCardanoAddress(added_account->account_id,
                                       mojom::CardanoKeyId::New(
                                           mojom::CardanoKeyRole::kExternal, 0))
                   ->address_string;
  auto addr2 = keyring_service()
                   ->GetCardanoAddress(added_account->account_id,
                                       mojom::CardanoKeyId::New(
                                           mojom::CardanoKeyRole::kExternal, 1))
                   ->address_string;

  EXPECT_EQ(request->origin_info->origin_spec, "https://brave.com");
  EXPECT_EQ(request->raw_tx_data, base::HexEncode(unsigned_tx_bytes));

  EXPECT_EQ(request->inputs.size(), 2u);
  EXPECT_EQ(request->inputs[0]->address, addr1);
  EXPECT_EQ(request->inputs[0]->value, 34451133u);

  EXPECT_EQ(request->inputs[1]->address, addr2);
  EXPECT_EQ(request->inputs[1]->value, 34451133u);

  EXPECT_EQ(request->outputs.size(), 3u);
  EXPECT_EQ(request->outputs[0]->address,
            "addr1q9zwt6rfn2e3mc63hesal6muyg807cwjnkwg3j5azkvmxm0tyqeyc8eu034zz"
            "mj4z53l7lh5u7z08l0rvp49ht88s5uskl6tsl");
  EXPECT_EQ(request->outputs[0]->value, 10000000u);

  EXPECT_EQ(request->outputs[1]->address,
            "addr1q8s90ehlgwwkq637d3r6qzuxwu6qnprphqadn9pjg2mtcp9hkfmyv4zfhyefv"
            "jmpww7f7w9gwem3x6gcm3ulw3kpcgws9sgrhg");
  EXPECT_EQ(request->outputs[1]->value, 24282816u);

  EXPECT_EQ(request->outputs[2]->address, addr1);
  EXPECT_EQ(request->outputs[2]->value, 24282816u);

  auto& api_signed_tx = future.Get<0>();
  auto& error = future.Get<1>();

  EXPECT_EQ(api_signed_tx.value(), base::HexEncode(signed_tx_bytes));
  EXPECT_FALSE(error);
}

TEST_F(CardanoApiImplTest, SignTx_ExistingExternalSignature) {
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

  CardanoTransaction unsigned_tx;
  SetupUnsignedReferenceTransaction(added_account, unsigned_tx);
  CardanoTransaction::TxWitness external_witness;
  external_witness.witness_bytes.fill(1);
  unsigned_tx.AddWitness(std::move(external_witness));

  auto unsigned_tx_bytes =
      CardanoTransactionSerializer().SerializeTransaction(unsigned_tx);

  auto tx_hash = CardanoTransactionSerializer().GetTxHash(unsigned_tx);
  std::vector<CardanoSignMessageResult> sign_results;
  sign_results.emplace_back(
      brave_wallet_service()
          ->keyring_service()
          ->SignMessageByCardanoKeyring(
              added_account->account_id,
              mojom::CardanoKeyId::New(mojom::CardanoKeyRole::kExternal, 0),
              tx_hash)
          .value());
  sign_results.emplace_back(
      brave_wallet_service()
          ->keyring_service()
          ->SignMessageByCardanoKeyring(
              added_account->account_id,
              mojom::CardanoKeyId::New(mojom::CardanoKeyRole::kExternal, 1),
              tx_hash)
          .value());

  CardanoTransaction signed_tx = unsigned_tx;
  for (const auto& sign_result : sign_results) {
    CardanoTransaction::TxWitness witness;
    auto span_writer = base::SpanWriter(base::span(witness.witness_bytes));
    span_writer.Write(sign_result.pubkey);
    span_writer.Write(sign_result.signature);
    signed_tx.AddWitness(std::move(witness));
  }

  auto signed_tx_bytes =
      CardanoTransactionSerializer().SerializeTransaction(signed_tx);

  TestFuture<const std::optional<std::string>&,
             mojom::CardanoProviderErrorBundlePtr>
      future;

  SignCardanoTransactionRequestWaiter waiter(brave_wallet_service());
  provider()->SignTx(base::HexEncode(unsigned_tx_bytes), false,
                     future.GetCallback());
  waiter.WaitAndProcess(true);

  auto& api_signed_tx = future.Get<0>();
  auto& error = future.Get<1>();

  EXPECT_EQ(api_signed_tx.value(), base::HexEncode(signed_tx_bytes));
  EXPECT_FALSE(error);
}

TEST_F(CardanoApiImplTest, SignTx_PartialSign) {
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

  CardanoTransaction unsigned_tx;
  SetupUnsignedReferenceTransaction(added_account, unsigned_tx);
  // Add an external input.
  CardanoTransaction::TxInput input;
  input.utxo_outpoint.txid.fill(55u);
  input.utxo_outpoint.index = 0;
  input.utxo_value = 34451133;
  unsigned_tx.AddInput(std::move(input));

  auto unsigned_tx_bytes =
      CardanoTransactionSerializer().SerializeTransaction(unsigned_tx);

  auto tx_hash = CardanoTransactionSerializer().GetTxHash(unsigned_tx);
  std::vector<CardanoSignMessageResult> sign_results;
  sign_results.emplace_back(
      brave_wallet_service()
          ->keyring_service()
          ->SignMessageByCardanoKeyring(
              added_account->account_id,
              mojom::CardanoKeyId::New(mojom::CardanoKeyRole::kExternal, 0),
              tx_hash)
          .value());
  sign_results.emplace_back(
      brave_wallet_service()
          ->keyring_service()
          ->SignMessageByCardanoKeyring(
              added_account->account_id,
              mojom::CardanoKeyId::New(mojom::CardanoKeyRole::kExternal, 1),
              tx_hash)
          .value());

  CardanoTransaction signed_tx = unsigned_tx;
  for (const auto& sign_result : sign_results) {
    CardanoTransaction::TxWitness witness;
    auto span_writer = base::SpanWriter(base::span(witness.witness_bytes));
    span_writer.Write(sign_result.pubkey);
    span_writer.Write(sign_result.signature);
    signed_tx.AddWitness(std::move(witness));
  }

  auto signed_tx_bytes =
      CardanoTransactionSerializer().SerializeTransaction(signed_tx);

  TestFuture<const std::optional<std::string>&,
             mojom::CardanoProviderErrorBundlePtr>
      future;

  SignCardanoTransactionRequestWaiter waiter(brave_wallet_service());
  provider()->SignTx(base::HexEncode(unsigned_tx_bytes), true,
                     future.GetCallback());
  auto request = waiter.WaitAndProcess(true);

  auto addr1 = keyring_service()
                   ->GetCardanoAddress(added_account->account_id,
                                       mojom::CardanoKeyId::New(
                                           mojom::CardanoKeyRole::kExternal, 0))
                   ->address_string;
  auto addr2 = keyring_service()
                   ->GetCardanoAddress(added_account->account_id,
                                       mojom::CardanoKeyId::New(
                                           mojom::CardanoKeyRole::kExternal, 1))
                   ->address_string;

  EXPECT_EQ(request->origin_info->origin_spec, "https://brave.com");
  EXPECT_EQ(request->raw_tx_data, base::HexEncode(unsigned_tx_bytes));

  EXPECT_EQ(request->inputs.size(), 3u);
  EXPECT_EQ(request->inputs[0]->address, addr1);
  EXPECT_EQ(request->inputs[0]->value, 34451133u);
  EXPECT_EQ(request->inputs[0]->outpoint_txid,
            "A7B4C1021FA375A4FCCB1AC1B3BB01743B3989B5EB732CC6240ADD8C71EDB925");
  EXPECT_EQ(request->inputs[0]->outpoint_index, 0u);

  EXPECT_EQ(request->inputs[1]->address, addr2);
  EXPECT_EQ(request->inputs[1]->value, 34451133u);
  EXPECT_EQ(request->inputs[1]->outpoint_txid,
            "A7B4C1021FA375A4FCCB1AC1B3BB01743B3989B5EB732CC6240ADD8C71EDB925");
  EXPECT_EQ(request->inputs[1]->outpoint_index, 1u);

  EXPECT_EQ(request->inputs[2]->address, "");
  EXPECT_EQ(request->inputs[2]->value, 0u);
  EXPECT_EQ(request->inputs[2]->outpoint_txid,
            "3737373737373737373737373737373737373737373737373737373737373737");
  EXPECT_EQ(request->inputs[2]->outpoint_index, 0u);

  EXPECT_EQ(request->outputs.size(), 3u);
  EXPECT_EQ(request->outputs[0]->address,
            "addr1q9zwt6rfn2e3mc63hesal6muyg807cwjnkwg3j5azkvmxm0tyqeyc8eu034zz"
            "mj4z53l7lh5u7z08l0rvp49ht88s5uskl6tsl");
  EXPECT_EQ(request->outputs[0]->value, 10000000u);

  EXPECT_EQ(request->outputs[1]->address,
            "addr1q8s90ehlgwwkq637d3r6qzuxwu6qnprphqadn9pjg2mtcp9hkfmyv4zfhyefv"
            "jmpww7f7w9gwem3x6gcm3ulw3kpcgws9sgrhg");
  EXPECT_EQ(request->outputs[1]->value, 24282816u);

  EXPECT_EQ(request->outputs[2]->address, addr1);
  EXPECT_EQ(request->outputs[2]->value, 24282816u);

  auto& api_signed_tx = future.Get<0>();
  auto& error = future.Get<1>();

  EXPECT_EQ(api_signed_tx.value(), base::HexEncode(signed_tx_bytes));
  EXPECT_FALSE(error);
}

}  // namespace brave_wallet
