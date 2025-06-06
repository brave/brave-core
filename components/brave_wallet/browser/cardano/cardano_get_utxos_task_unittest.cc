/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_get_utxos_task.h"

#include <memory>
#include <optional>
#include <string>

#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
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

class CardanoGetUtxosTaskUnitTest : public testing::Test {
 public:
  CardanoGetUtxosTaskUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  ~CardanoGetUtxosTaskUnitTest() override = default;

  void SetUp() override {
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
    SetupCardanoAccount();
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
  base::test::ScopedFeatureList scoped_cardano_feature_{
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

TEST_F(CardanoGetUtxosTaskUnitTest, OneAddress) {
  auto addresses = *keyring_service_->GetCardanoAddresses(account_id());

  auto addr1 = *CardanoAddress::FromString(addresses[0]->address_string);

  GetCardanoUtxosTask task(*cardano_wallet_service_, mojom::kCardanoMainnet,
                           {addr1});

  TestFuture<base::expected<GetCardanoUtxosTask::UtxoMap, std::string>>
      utxos_future;
  task.Start(utxos_future.GetCallback());

  auto utxos = utxos_future.Take().value();
  EXPECT_EQ(utxos.size(), 1u);
  EXPECT_EQ(utxos[addr1].size(), 2u);

  EXPECT_EQ(utxos[addr1][0].lovelace_amount, 54321u);
  EXPECT_EQ(utxos[addr1][1].lovelace_amount, 600000u);
}

TEST_F(CardanoGetUtxosTaskUnitTest, TwoAddresses) {
  auto addresses = *keyring_service_->GetCardanoAddresses(account_id());

  auto addr1 = *CardanoAddress::FromString(addresses[0]->address_string);
  auto addr2 = *CardanoAddress::FromString(addresses[1]->address_string);

  GetCardanoUtxosTask task(*cardano_wallet_service_, mojom::kCardanoMainnet,
                           {addr1, addr2});

  TestFuture<base::expected<GetCardanoUtxosTask::UtxoMap, std::string>>
      utxos_future;
  task.Start(utxos_future.GetCallback());

  auto utxos = utxos_future.Take().value();
  EXPECT_EQ(utxos.size(), 2u);

  EXPECT_EQ(utxos[addr1].size(), 2u);
  EXPECT_EQ(utxos[addr1][0].lovelace_amount, 54321u);
  EXPECT_EQ(utxos[addr1][1].lovelace_amount, 600000u);

  EXPECT_EQ(utxos[addr2].size(), 1u);
  EXPECT_EQ(utxos[addr2][0].lovelace_amount, 7000000u);
}

TEST_F(CardanoGetUtxosTaskUnitTest, NetworkFailure) {
  auto addresses = *keyring_service_->GetCardanoAddresses(account_id());

  auto addr1 = *CardanoAddress::FromString(addresses[0]->address_string);

  GetCardanoUtxosTask task(*cardano_wallet_service_, mojom::kCardanoMainnet,
                           {addr1});

  cardano_test_rpc_server_->set_fail_address_utxo_request(true);

  TestFuture<base::expected<GetCardanoUtxosTask::UtxoMap, std::string>>
      utxos_future;
  task.Start(utxos_future.GetCallback());

  EXPECT_EQ(utxos_future.Take().error(), WalletInternalErrorMessage());
}

TEST_F(CardanoGetUtxosTaskUnitTest, UnknownAddress) {
  auto account_id = GetAccountUtils()
                        .EnsureAccount(mojom::KeyringId::kCardanoMainnet, 33)
                        ->account_id.Clone();

  auto addresses = *keyring_service_->GetCardanoAddresses(account_id);

  auto addr1 = *CardanoAddress::FromString(addresses[0]->address_string);

  GetCardanoUtxosTask task(*cardano_wallet_service_, mojom::kCardanoMainnet,
                           {addr1});

  TestFuture<base::expected<GetCardanoUtxosTask::UtxoMap, std::string>>
      utxos_future;
  task.Start(utxos_future.GetCallback());

  auto utxos = utxos_future.Take().value();
  EXPECT_EQ(utxos.size(), 1u);

  EXPECT_EQ(utxos[addr1].size(), 0u);
}

}  // namespace brave_wallet
