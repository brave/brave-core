/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_create_transaction_task.h"

#include <memory>
#include <optional>
#include <string>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
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

class CardanoCreateTransactionTaskUnitTest : public testing::Test {
 public:
  CardanoCreateTransactionTaskUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  ~CardanoCreateTransactionTaskUnitTest() override = default;

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
    SetupCardanoAccount();
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

TEST_F(CardanoCreateTransactionTaskUnitTest, FixedAmount) {
  CardanoCreateTransactionTask task(
      *cardano_wallet_service_, account_id(),
      *CardanoAddress::FromString(kMockCardanoAddress1), 1000000, false);

  TestFuture<base::expected<CardanoTransaction, std::string>> tx_future;
  task.Start(tx_future.GetCallback());

  auto tx = tx_future.Take().value();
  EXPECT_EQ(tx.TargetOutput()->amount, 1000000u);
  EXPECT_EQ(tx.ChangeOutput()->amount, 5831859u);
  EXPECT_EQ(tx.EffectiveFeeAmount(), 168141u);
}

TEST_F(CardanoCreateTransactionTaskUnitTest, MaxValue) {
  CardanoCreateTransactionTask task(
      *cardano_wallet_service_, account_id(),
      *CardanoAddress::FromString(kMockCardanoAddress1), 0, true);

  TestFuture<base::expected<CardanoTransaction, std::string>> tx_future;
  task.Start(tx_future.GetCallback());

  auto tx = tx_future.Take().value();
  EXPECT_EQ(tx.TargetOutput()->amount, 7476808u);
  EXPECT_EQ(tx.ChangeOutput(), nullptr);
  EXPECT_EQ(tx.EffectiveFeeAmount(), 177513u);
}

TEST_F(CardanoCreateTransactionTaskUnitTest, InsufficientBalance) {
  CardanoCreateTransactionTask task(
      *cardano_wallet_service_, account_id(),
      *CardanoAddress::FromString(kMockCardanoAddress1), 10000000, false);

  TestFuture<base::expected<CardanoTransaction, std::string>> tx_future;
  task.Start(tx_future.GetCallback());

  EXPECT_EQ(tx_future.Take().error(), WalletInsufficientBalanceErrorMessage());
}

TEST_F(CardanoCreateTransactionTaskUnitTest, EmptyAccount) {
  CardanoCreateTransactionTask task(
      *cardano_wallet_service_,
      GetAccountUtils()
          .EnsureAccount(mojom::KeyringId::kCardanoMainnet, 33)
          ->account_id,
      *CardanoAddress::FromString(kMockCardanoAddress1), 10000000, false);

  TestFuture<base::expected<CardanoTransaction, std::string>> tx_future;
  task.Start(tx_future.GetCallback());

  EXPECT_EQ(tx_future.Take().error(), WalletInsufficientBalanceErrorMessage());
}

TEST_F(CardanoCreateTransactionTaskUnitTest, FailedLatestEpochParameters) {
  CardanoCreateTransactionTask task(
      *cardano_wallet_service_, account_id(),
      *CardanoAddress::FromString(kMockCardanoAddress1), 1000000, false);

  cardano_test_rpc_server_->set_fail_latest_epoch_parameters_request(true);
  TestFuture<base::expected<CardanoTransaction, std::string>> tx_future;
  task.Start(tx_future.GetCallback());

  EXPECT_EQ(tx_future.Take().error(), WalletInternalErrorMessage());
}

TEST_F(CardanoCreateTransactionTaskUnitTest, FailedLatestBlock) {
  CardanoCreateTransactionTask task(
      *cardano_wallet_service_, account_id(),
      *CardanoAddress::FromString(kMockCardanoAddress1), 1000000, false);

  cardano_test_rpc_server_->set_fail_latest_block_request(true);
  TestFuture<base::expected<CardanoTransaction, std::string>> tx_future;
  task.Start(tx_future.GetCallback());

  EXPECT_EQ(tx_future.Take().error(), WalletInternalErrorMessage());
}

TEST_F(CardanoCreateTransactionTaskUnitTest, FailedUtxo) {
  CardanoCreateTransactionTask task(
      *cardano_wallet_service_, account_id(),
      *CardanoAddress::FromString(kMockCardanoAddress1), 1000000, false);

  cardano_test_rpc_server_->set_fail_address_utxo_request(true);
  TestFuture<base::expected<CardanoTransaction, std::string>> tx_future;
  task.Start(tx_future.GetCallback());

  EXPECT_EQ(tx_future.Take().error(), WalletInternalErrorMessage());
}

}  // namespace brave_wallet
