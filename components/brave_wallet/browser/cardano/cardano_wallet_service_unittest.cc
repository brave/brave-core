/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"

#include <memory>
#include <optional>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_schema.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"  // IWYU pragma: keep
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"  // IWYU pragma: keep
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::TestFuture;

namespace brave_wallet {

namespace {

constexpr char kExternal0Address[] =
    "addr1q9gn9ra9l2mz35uc0ww0qkgf5mugczqyxvr5wegdacxa724hwphl5wrg6u8s8"
    "cxpy8vz4k2g73yc9nzvalpwnvgmkxpq6jdpa8";

}

class CardanoWalletServiceUnitTest : public testing::Test {
 public:
  CardanoWalletServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  ~CardanoWalletServiceUnitTest() override = default;

  void SetUp() override {
    CardanoCreateTransactionTask::SetArrangeTransactionForTesting(true);

    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());

    network_manager_ = std::make_unique<NetworkManager>(&prefs_);

    keyring_service_ =
        std::make_unique<KeyringService>(nullptr, &prefs_, &local_state_);
    cardano_wallet_service_ = std::make_unique<CardanoWalletService>(
        *keyring_service_, *network_manager_, nullptr);
    cardano_test_rpc_server_ =
        std::make_unique<CardanoTestRpcServer>(*cardano_wallet_service_);

    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, kTestWalletPassword);
  }

  void TearDown() override {
    CardanoCreateTransactionTask::SetArrangeTransactionForTesting(false);
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  void SetupCardanoAccount(uint32_t next_external_index = 0,
                           uint32_t next_internal_index = 0) {
    cardano_test_rpc_server_->SetUpCardanoRpc(kMnemonicDivideCruise, 0);
    cardano_account_ =
        GetAccountUtils().EnsureAccount(mojom::KeyringId::kCardanoMainnet, 0);
    ASSERT_TRUE(cardano_account_);
    keyring_service_->UpdateNextUnusedAddressForCardanoAccount(
        cardano_account_->account_id, next_external_index, next_internal_index);
  }

  mojom::AccountIdPtr account_id() const {
    return cardano_account_->account_id.Clone();
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_{
      features::kBraveWalletCardanoFeature};

  mojom::AccountInfoPtr cardano_account_;

  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<CardanoWalletService> cardano_wallet_service_;
  std::unique_ptr<CardanoTestRpcServer> cardano_test_rpc_server_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(CardanoWalletServiceUnitTest, GetBalance) {
  SetupCardanoAccount();

  TestFuture<mojom::CardanoBalancePtr, const std::optional<std::string>&>
      balance_future;

  cardano_wallet_service_->GetBalance(account_id(), std::nullopt,
                                      balance_future.GetCallback());

  auto [balance, error] = balance_future.Take();
  EXPECT_FALSE(error);
  EXPECT_EQ(balance->total_balance, 969750u + 2000000u + 7000000u);
}

TEST_F(CardanoWalletServiceUnitTest, GetBalanceForToken) {
  SetupCardanoAccount();
  auto& token1 = cardano_test_rpc_server_->utxo_map()[kExternal0Address][0]
                     .amount.emplace_back();
  token1.quantity = "12345";
  token1.unit =
      "29d222ce763455e3d7a09a665ce554f00ac89d2e99a1a83d267170c64d494e";

  auto& token2 = cardano_test_rpc_server_->utxo_map()[kExternal0Address][0]
                     .amount.emplace_back();
  token2.quantity = "100";
  token2.unit =
      "8bca871dcd4f1dd9588d901d02677b6d2413fd19e4c8febfc353edff5045505041";

  auto& token3 = cardano_test_rpc_server_->utxo_map()[kExternal0Address][1]
                     .amount.emplace_back();
  token3.quantity = "200";
  token3.unit =
      "8bca871dcd4f1dd9588d901d02677b6d2413fd19e4c8febfc353edff5045505041";

  TestFuture<mojom::CardanoBalancePtr, const std::optional<std::string>&>
      balance_future;

  cardano_wallet_service_->GetBalance(
      account_id(),
      "29d222ce763455e3d7a09a665ce554f00ac89d2e99a1a83d267170c64d494e",
      balance_future.GetCallback());

  auto [balance, error] = balance_future.Take();
  EXPECT_FALSE(error);
  EXPECT_EQ(balance->total_balance, 12345u);

  cardano_wallet_service_->GetBalance(
      account_id(),
      "8bca871dcd4f1dd9588d901d02677b6d2413fd19e4c8febfc353edff5045505041",
      balance_future.GetCallback());

  std::tie(balance, error) = balance_future.Take();
  EXPECT_FALSE(error);
  EXPECT_EQ(balance->total_balance, 100u + 200u);

  // Lovelace balance still works.
  cardano_wallet_service_->GetBalance(account_id(), std::nullopt,
                                      balance_future.GetCallback());

  std::tie(balance, error) = balance_future.Take();
  EXPECT_FALSE(error);
  EXPECT_EQ(balance->total_balance, 969750u + 2000000u + 7000000u);
}

TEST_F(CardanoWalletServiceUnitTest, GetUtxos) {
  SetupCardanoAccount();
  auto& token1 = cardano_test_rpc_server_->utxo_map()[kExternal0Address][0]
                     .amount.emplace_back();
  token1.quantity = "12345";
  token1.unit = base::HexEncodeLower(GetMockTokenId("foo"));

  auto& token2 = cardano_test_rpc_server_->utxo_map()[kExternal0Address][0]
                     .amount.emplace_back();
  token2.quantity = "100";
  token2.unit = base::HexEncodeLower(GetMockTokenId("bar"));

  auto& token3 = cardano_test_rpc_server_->utxo_map()[kExternal0Address][1]
                     .amount.emplace_back();
  token3.quantity = "200";
  token3.unit = base::HexEncodeLower(GetMockTokenId("bar"));

  TestFuture<base::expected<cardano_rpc::UnspentOutputs, std::string>>
      utxos_future;

  cardano_wallet_service_->GetUtxos(account_id(), utxos_future.GetCallback());

  auto utxos = utxos_future.Take();
  EXPECT_TRUE(utxos.has_value());
  EXPECT_EQ(utxos->size(), 3u);

  EXPECT_EQ(utxos->at(0).lovelace_amount, 969750u);
  EXPECT_EQ(utxos->at(0).address_to.ToString(),
            "addr1q9gn9ra9l2mz35uc0ww0qkgf5mugczqyxvr5wegdacxa724hwphl5wrg6u8s8"
            "cxpy8vz4k2g73yc9nzvalpwnvgmkxpq6jdpa8");
  EXPECT_EQ(utxos->at(0).output_index, 13u);
  EXPECT_EQ(utxos->at(0).tokens,
            cardano_rpc::Tokens({{GetMockTokenId("foo"), 12345u},
                                 {GetMockTokenId("bar"), 100u}}));
  EXPECT_EQ(base::HexEncodeLower(utxos->at(0).tx_hash),
            "0000000000000000000000000000000000000000000000000000000000000000");

  EXPECT_EQ(utxos->at(1).lovelace_amount, 2000000u);
  EXPECT_EQ(utxos->at(1).address_to.ToString(),
            "addr1q9gn9ra9l2mz35uc0ww0qkgf5mugczqyxvr5wegdacxa724hwphl5wrg6u8s8"
            "cxpy8vz4k2g73yc9nzvalpwnvgmkxpq6jdpa8");
  EXPECT_EQ(utxos->at(1).output_index, 13u);
  EXPECT_EQ(utxos->at(1).tokens,
            cardano_rpc::Tokens({{GetMockTokenId("bar"), 200u}}));
  EXPECT_EQ(base::HexEncodeLower(utxos->at(1).tx_hash),
            "0100000000000000000000000000000000000000000000000000000000000000");

  EXPECT_EQ(utxos->at(2).lovelace_amount, 7000000u);
  EXPECT_EQ(utxos->at(2).address_to.ToString(),
            "addr1q99ed78r27qyyqdmmce2gupzzeansmnf70jake3cmgt4hjahwphl5wrg6u8s8"
            "cxpy8vz4k2g73yc9nzvalpwnvgmkxpqv7f05j");
  EXPECT_EQ(utxos->at(2).output_index, 13u);
  EXPECT_EQ(utxos->at(2).tokens, cardano_rpc::Tokens{});
  EXPECT_EQ(base::HexEncodeLower(utxos->at(2).tx_hash),
            "0200000000000000000000000000000000000000000000000000000000000000");
}

TEST_F(CardanoWalletServiceUnitTest, GetUtxosRunsTokenDiscovery) {
  SetupCardanoAccount();
  auto& token1 = cardano_test_rpc_server_->utxo_map()[kExternal0Address][0]
                     .amount.emplace_back();
  token1.quantity = "12345";
  token1.unit = base::HexEncodeLower(GetMockTokenId("foo"));

  auto& token2 = cardano_test_rpc_server_->utxo_map()[kExternal0Address][0]
                     .amount.emplace_back();
  token2.quantity = "100";
  token2.unit = base::HexEncodeLower(GetMockTokenId("bar"));

  auto& token3 = cardano_test_rpc_server_->utxo_map()[kExternal0Address][1]
                     .amount.emplace_back();
  token3.quantity = "200";
  token3.unit = base::HexEncodeLower(GetMockTokenId("bar"));

  auto& foo_asset = cardano_test_rpc_server_->assets().emplace_back();
  foo_asset.asset = base::HexEncodeLower(GetMockTokenId("foo"));
  foo_asset.metadata.decimals = 6;
  foo_asset.metadata.name = "Foo token";
  foo_asset.metadata.ticker = "foo";

  auto& bar_asset = cardano_test_rpc_server_->assets().emplace_back();
  bar_asset.asset = base::HexEncodeLower(GetMockTokenId("bar"));
  bar_asset.metadata.decimals = 6;
  bar_asset.metadata.name = "Bar token";
  bar_asset.metadata.ticker = "bar";

  TestFuture<mojom::BlockchainTokenPtr> token_discovery_future;
  cardano_wallet_service_->SetNewTokenDiscoveredCallback(
      token_discovery_future.GetRepeatingCallback());

  TestFuture<base::expected<cardano_rpc::UnspentOutputs, std::string>>
      utxos_future;

  cardano_wallet_service_->GetUtxos(account_id(), utxos_future.GetCallback());
  EXPECT_EQ(utxos_future.Take()->size(), 3u);
  EXPECT_TRUE(cardano_wallet_service_->GetCardanoRpc(mojom::kCardanoMainnet)
                  ->HasPendingRequestsForTesting());

  auto token_bar = token_discovery_future.Take();
  EXPECT_EQ(
      token_bar,
      mojom::BlockchainToken::New(
          "62626262626262626262626262626262626262626262626262626262626172",
          "Bar token", "", false, false, false, false,
          mojom::SPLTokenProgram::kUnknown, false, false, "bar", 6, true, "",
          "", mojom::kCardanoMainnet, mojom::CoinType::ADA, false));

  auto token_foo = token_discovery_future.Take();
  EXPECT_EQ(
      token_foo,
      mojom::BlockchainToken::New(
          "66666666666666666666666666666666666666666666666666666666666f6f",
          "Foo token", "", false, false, false, false,
          mojom::SPLTokenProgram::kUnknown, false, false, "foo", 6, true, "",
          "", mojom::kCardanoMainnet, mojom::CoinType::ADA, false));

  // Request same set of utxos again, no tokens appear.
  cardano_wallet_service_->GetUtxos(account_id(), utxos_future.GetCallback());
  EXPECT_EQ(utxos_future.Take()->size(), 3u);
  EXPECT_FALSE(cardano_wallet_service_->GetCardanoRpc(mojom::kCardanoMainnet)
                   ->HasPendingRequestsForTesting());

  // Add a new token to one of utxos.
  auto& token4 = cardano_test_rpc_server_->utxo_map()[kExternal0Address][1]
                     .amount.emplace_back();
  token4.quantity = "4200";
  token4.unit = base::HexEncodeLower(GetMockTokenId("baz"));
  auto& baz_asset = cardano_test_rpc_server_->assets().emplace_back();
  baz_asset.asset = base::HexEncodeLower(GetMockTokenId("baz"));
  baz_asset.metadata.decimals = 6;
  baz_asset.metadata.name = "Baz token";
  baz_asset.metadata.ticker = "baz";

  // Request utxos - new token will be discovered.
  cardano_wallet_service_->GetUtxos(account_id(), utxos_future.GetCallback());
  EXPECT_EQ(utxos_future.Take()->size(), 3u);
  EXPECT_TRUE(cardano_wallet_service_->GetCardanoRpc(mojom::kCardanoMainnet)
                  ->HasPendingRequestsForTesting());

  auto token_baz = token_discovery_future.Take();
  EXPECT_EQ(
      token_baz,
      mojom::BlockchainToken::New(
          "6262626262626262626262626262626262626262626262626262626262617a",
          "Baz token", "", false, false, false, false,
          mojom::SPLTokenProgram::kUnknown, false, false, "baz", 6, true, "",
          "", mojom::kCardanoMainnet, mojom::CoinType::ADA, false));
}

TEST_F(CardanoWalletServiceUnitTest, GetTransactionStatus) {
  SetupCardanoAccount();

  TestFuture<base::expected<bool, std::string>> tx_status_future;

  cardano_wallet_service_->GetTransactionStatus(
      mojom::kCardanoMainnet, kMockCardanoTxid, tx_status_future.GetCallback());

  EXPECT_FALSE(tx_status_future.Take().value());

  cardano_test_rpc_server_->AddConfirmedTransaction(kMockCardanoTxid);

  cardano_wallet_service_->GetTransactionStatus(
      mojom::kCardanoMainnet, kMockCardanoTxid, tx_status_future.GetCallback());

  EXPECT_TRUE(tx_status_future.Take().value());
}

TEST_F(CardanoWalletServiceUnitTest, GetUsedAddresses) {
  SetupCardanoAccount();
  auto addresses = cardano_wallet_service_->GetUsedAddresses(account_id());
  EXPECT_EQ(addresses.size(), 1u);
  EXPECT_EQ(addresses[0]->address_string, kExternal0Address);
}

TEST_F(CardanoWalletServiceUnitTest, GetUnusedAddresses) {
  SetupCardanoAccount();

  EXPECT_EQ(0u,
            cardano_wallet_service_->GetUnusedAddresses(account_id()).size());
}

TEST_F(CardanoWalletServiceUnitTest, GetChangeAddress) {
  SetupCardanoAccount();
  auto address = cardano_wallet_service_->GetChangeAddress(account_id());
  EXPECT_EQ(address->address_string, kExternal0Address);
}

TEST_F(CardanoWalletServiceUnitTest,
       GetUsedAddresses_ReturnsSingleAddressOnly) {
  SetupCardanoAccount();
  auto addresses = cardano_wallet_service_->GetUsedAddresses(account_id());

  ASSERT_EQ(addresses.size(), 1u);
  EXPECT_EQ(addresses[0]->payment_key_id->role,
            mojom::CardanoKeyRole::kExternal);
  EXPECT_EQ(addresses[0]->payment_key_id->index, 0u);
  EXPECT_EQ(addresses[0]->address_string,
            "addr1q9gn9ra9l2mz35uc0ww0qkgf5mugczqyxvr5wegdacxa724hwphl5wrg6u8s8"
            "cxpy8vz4k2g73yc9nzvalpwnvgmkxpq6jdpa8");
}

TEST_F(CardanoWalletServiceUnitTest, GetUnusedAddresses_ReturnsEmptyArray) {
  SetupCardanoAccount();
  auto addresses = cardano_wallet_service_->GetUnusedAddresses(account_id());

  EXPECT_EQ(addresses.size(), 0u);
}

TEST_F(CardanoWalletServiceUnitTest,
       GetChangeAddress_ReturnsSameAsUsedAddress) {
  SetupCardanoAccount();

  auto used_addresses = cardano_wallet_service_->GetUsedAddresses(account_id());
  auto change_address = cardano_wallet_service_->GetChangeAddress(account_id());

  ASSERT_EQ(used_addresses.size(), 1u);
  ASSERT_TRUE(change_address);
  EXPECT_EQ(change_address->address_string, used_addresses[0]->address_string);
  EXPECT_EQ(change_address->payment_key_id->role,
            mojom::CardanoKeyRole::kExternal);
  EXPECT_EQ(change_address->payment_key_id->index, 0u);
}

TEST_F(CardanoWalletServiceUnitTest, AllAddressMethods_ReturnSameAddress) {
  SetupCardanoAccount();

  auto used_addresses = cardano_wallet_service_->GetUsedAddresses(account_id());
  auto change_address = cardano_wallet_service_->GetChangeAddress(account_id());

  ASSERT_EQ(used_addresses.size(), 1u);
  ASSERT_TRUE(change_address);

  const std::string expected_address =
      "addr1q9gn9ra9l2mz35uc0ww0qkgf5mugczqyxvr5wegdacxa724hwphl5wrg6u8s8"
      "cxpy8vz4k2g73yc9nzvalpwnvgmkxpq6jdpa8";

  EXPECT_EQ(used_addresses[0]->address_string, expected_address);
  EXPECT_EQ(change_address->address_string, expected_address);
  EXPECT_EQ(used_addresses[0]->payment_key_id->role,
            mojom::CardanoKeyRole::kExternal);
  EXPECT_EQ(used_addresses[0]->payment_key_id->index, 0u);
  EXPECT_EQ(change_address->payment_key_id->role,
            mojom::CardanoKeyRole::kExternal);
  EXPECT_EQ(change_address->payment_key_id->index, 0u);
}

TEST_F(CardanoWalletServiceUnitTest, NoAddressMethod_ReturnsMultipleAddresses) {
  SetupCardanoAccount();

  auto used_addresses = cardano_wallet_service_->GetUsedAddresses(account_id());
  EXPECT_EQ(used_addresses.size(), 1u);

  auto unused_addresses =
      cardano_wallet_service_->GetUnusedAddresses(account_id());
  EXPECT_EQ(unused_addresses.size(), 0u);

  EXPECT_LE(used_addresses.size(), 1u);

  EXPECT_EQ(unused_addresses.size(), 0u);
}

TEST_F(CardanoWalletServiceUnitTest, CreateAndSignCardanoTransaction) {
  // TODO(https://github.com/brave/brave-browser/issues/45278): needs more tests
  // for all corner cases.
  SetupCardanoAccount();

  TestFuture<base::expected<CardanoTransaction, std::string>> create_tx_future;

  cardano_wallet_service_->CreateCardanoTransaction(
      account_id(), *CardanoAddress::FromString(kMockCardanoAddress1), 8800000,
      false, create_tx_future.GetCallback());

  auto captured_tx = create_tx_future.Take();

  ASSERT_TRUE(captured_tx.has_value());

  EXPECT_EQ(captured_tx->inputs().size(), 3u);
  EXPECT_EQ(captured_tx->outputs().size(), 2u);
  EXPECT_EQ(captured_tx->GetTotalInputsAmount().ValueOrDie(), 9969750u);
  EXPECT_EQ(captured_tx->GetTotalOutputsAmount().ValueOrDie(),
            9969750u - 176017);
  EXPECT_EQ(captured_tx->fee(), 176017u);
  EXPECT_EQ(captured_tx->invalid_after(), 155486947u);

  TestFuture<std::string, CardanoTransaction, std::string> post_future;

  cardano_wallet_service_->SignAndPostTransaction(
      account_id(), captured_tx.value(), post_future.GetCallback());

  auto [txid, signed_tx, error] = post_future.Take();
  EXPECT_EQ(signed_tx.witnesses().size(), 2u);
  EXPECT_EQ(error, "");

  EXPECT_EQ(
      "84A400D90102838258200000000000000000000000000000000000000000000000000000"
      "0000000000000D8258200100000000000000000000000000000000000000000000000000"
      "0000000000000D8258200200000000000000000000000000000000000000000000000000"
      "0000000000000D01828258390144E5E8699AB31DE351BE61DFEB7C220EFF61D29D9C88CA"
      "9D1599B36DEB20324C1F3C7C6A216E551523FF7EF4E784F3FDE3606A5BACE785391A0086"
      "47008258390151328FA5FAB628D3987B9CF05909A6F88C0804330747650DEE0DDF2AB770"
      "6FFA3868D70F03E0C121D82AD948F44982CC4CEFC2E9B11BB1821A000F29C5021A0002AF"
      "91031A09448AE3A100D901028282582039F9A9705B72246693CDACE42F68901109C80536"
      "2A98038749E2FF6ECA6BEBE3584073E656D2E6F6ABA003DEA6ED5E52CD3164A5F129F89B"
      "A4CA3E7579F087475CF77E70E51F1ADD9D041D067ABECF72945DDD086F267C08D1CD81A5"
      "4A29DAC9F20A825820D9E38698F13131246B9234BBDDE147AFBA999E34EFF03EEADDA5A3"
      "36ABCA7296584035CD7F035C4C7BA9B930CE2319FB7369358110622E9E297230BB0B4C71"
      "1F4D6196FF7CAB4458419710CED215A4ADD19B969A9359A3E2F98A94BAD92A638FC202F5"
      "F6",
      cardano_test_rpc_server_->captured_raw_tx());
}

TEST_F(CardanoWalletServiceUnitTest,
       SignAndPostTransaction_RejectsAfterWalletLock) {
  SetupCardanoAccount();

  base::MockCallback<CardanoWalletService::CardanoCreateTransactionTaskCallback>
      create_callback;

  base::expected<CardanoTransaction, std::string> captured_tx;

  EXPECT_CALL(create_callback, Run(_))
      .WillOnce(DoAll(SaveArg<0>(&captured_tx),
                      RunOnceClosure(task_environment_.QuitClosure())));

  cardano_wallet_service_->CreateCardanoTransaction(
      account_id(), *CardanoAddress::FromString(kMockCardanoAddress1), 7400000,
      false, create_callback.Get());
  task_environment_.RunUntilQuit();
  testing::Mock::VerifyAndClearExpectations(&create_callback);

  ASSERT_TRUE(captured_tx.has_value());

  keyring_service_->Lock();
  EXPECT_TRUE(keyring_service_->IsLockedSync());

  base::MockCallback<CardanoWalletService::SignAndPostTransactionCallback>
      post_callback;

  std::string captured_error;
  EXPECT_CALL(post_callback, Run(_, _, _))
      .WillOnce(DoAll(SaveArg<2>(&captured_error),
                      RunOnceClosure(task_environment_.QuitClosure())));

  cardano_wallet_service_->SignAndPostTransaction(
      account_id(), captured_tx.value(), post_callback.Get());

  task_environment_.RunUntilQuit();
  testing::Mock::VerifyAndClearExpectations(&post_callback);

  EXPECT_FALSE(captured_error.empty());
}

TEST_F(CardanoWalletServiceUnitTest,
       SignAndPostTransaction_HandlesLargeTransaction) {
  SetupCardanoAccount();

  base::MockCallback<CardanoWalletService::CardanoCreateTransactionTaskCallback>
      create_callback;

  base::expected<CardanoTransaction, std::string> captured_result;

  EXPECT_CALL(create_callback, Run(_))
      .WillOnce(DoAll(SaveArg<0>(&captured_result),
                      RunOnceClosure(task_environment_.QuitClosure())));

  cardano_wallet_service_->CreateCardanoTransaction(
      account_id(), *CardanoAddress::FromString(kMockCardanoAddress1),
      45000000000000000, false, create_callback.Get());
  task_environment_.RunUntilQuit();
  testing::Mock::VerifyAndClearExpectations(&create_callback);

  if (captured_result.has_value()) {
    base::MockCallback<CardanoWalletService::SignAndPostTransactionCallback>
        post_callback;

    EXPECT_CALL(post_callback, Run(_, _, _))
        .WillOnce(RunOnceClosure(task_environment_.QuitClosure()));

    cardano_wallet_service_->SignAndPostTransaction(
        account_id(), captured_result.value(), post_callback.Get());

    task_environment_.RunUntilQuit();
    testing::Mock::VerifyAndClearExpectations(&post_callback);
  } else {
    EXPECT_FALSE(captured_result.error().empty());
  }
}
}  // namespace brave_wallet
