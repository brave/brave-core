/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"

#include <memory>
#include <optional>
#include <string>

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
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

}  // namespace brave_wallet
