/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_wallet_service.h"

#include <memory>
#include <optional>
#include <string>

#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
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

using base::test::RunOnceClosure;
using testing::_;
using testing::DoAll;
using testing::SaveArg;
using testing::Truly;

namespace brave_wallet {

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
    task_environment_.RunUntilIdle();
    keyring_service_->UpdateNextUnusedAddressForCardanoAccount(
        cardano_account_->account_id, next_external_index, next_internal_index);
  }

  mojom::AccountIdPtr account_id() const {
    return cardano_account_->account_id.Clone();
  }

 protected:
  base::test::ScopedFeatureList scoped_btc_feature_{
      features::kBraveWalletCardanoFeature};

  mojom::AccountInfoPtr cardano_account_;
  mojom::AccountInfoPtr hw_btc_account_;

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

  base::MockCallback<CardanoWalletService::GetBalanceCallback> callback;

  auto expected_balance = mojom::CardanoBalance::New();
  expected_balance->total_balance = 969750u + 2000000u + 7000000u;

  EXPECT_CALL(callback, Run(Truly([&](const mojom::CardanoBalancePtr& balance) {
                              EXPECT_EQ(balance, expected_balance);
                              return true;
                            }),
                            std::optional<std::string>()));
  cardano_wallet_service_->GetBalance(account_id(), callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(CardanoWalletServiceUnitTest, GetTransactionStatus) {
  SetupCardanoAccount();

  base::MockCallback<CardanoWalletService::GetTransactionStatusCallback>
      callback;

  EXPECT_CALL(callback,
              Run(Truly([&](const base::expected<bool, std::string>& tx) {
                EXPECT_FALSE(tx.value());
                return true;
              })))
      .WillOnce(RunOnceClosure(task_environment_.QuitClosure()));
  cardano_wallet_service_->GetTransactionStatus(
      mojom::kCardanoMainnet, kMockCardanoTxid, callback.Get());
  task_environment_.RunUntilQuit();
  testing::Mock::VerifyAndClearExpectations(&callback);

  cardano_test_rpc_server_->AddConfirmedTransaction(kMockCardanoTxid);

  EXPECT_CALL(callback,
              Run(Truly([&](const base::expected<bool, std::string>& tx) {
                EXPECT_TRUE(tx.value());
                return true;
              })))
      .WillOnce(RunOnceClosure(task_environment_.QuitClosure()));
  cardano_wallet_service_->GetTransactionStatus(
      mojom::kCardanoMainnet, kMockCardanoTxid, callback.Get());
  task_environment_.RunUntilQuit();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

TEST_F(CardanoWalletServiceUnitTest, GetUsedAddresses) {
  SetupCardanoAccount();
  auto addresses = cardano_wallet_service_->GetUsedAddresses(account_id());
  EXPECT_EQ(addresses.size(), 1u);
  EXPECT_EQ(addresses[0]->address_string,
            "addr1q9gn9ra9l2mz35uc0ww0qkgf5mugczqyxvr5wegdacxa724hwphl5wrg6u8s8"
            "cxpy8vz4k2g73yc9nzvalpwnvgmkxpq6jdpa8");
}

TEST_F(CardanoWalletServiceUnitTest, GetUnusedAddresses) {
  SetupCardanoAccount();

  EXPECT_EQ(0u,
            cardano_wallet_service_->GetUnusedAddresses(account_id()).size());
}

TEST_F(CardanoWalletServiceUnitTest, GetChangeAddress) {
  SetupCardanoAccount();
  auto address = cardano_wallet_service_->GetChangeAddress(account_id());
  EXPECT_EQ(address->address_string,
            "addr1q9gn9ra9l2mz35uc0ww0qkgf5mugczqyxvr5wegdacxa724hwphl5wrg6u8s8"
            "cxpy8vz4k2g73yc9nzvalpwnvgmkxpq6jdpa8");
}

TEST_F(CardanoWalletServiceUnitTest, CreateAndSignCardanoTransaction) {
  // TODO(https://github.com/brave/brave-browser/issues/45278): needs more tests
  // for all corner cases.
  SetupCardanoAccount();

  base::MockCallback<CardanoWalletService::CardanoCreateTransactionTaskCallback>
      callback;

  base::expected<CardanoTransaction, std::string> captured_tx;

  EXPECT_CALL(callback, Run(_))
      .WillOnce(DoAll(SaveArg<0>(&captured_tx),
                      RunOnceClosure(task_environment_.QuitClosure())));
  cardano_wallet_service_->CreateCardanoTransaction(
      account_id(), *CardanoAddress::FromString(kMockCardanoAddress1), 7400000,
      false, callback.Get());
  task_environment_.RunUntilQuit();
  testing::Mock::VerifyAndClearExpectations(&callback);

  ASSERT_TRUE(captured_tx.has_value());

  EXPECT_EQ(captured_tx->GetTotalInputsAmount().ValueOrDie(), 9000000u);
  EXPECT_EQ(captured_tx->GetTotalOutputsAmount().ValueOrDie(), 8825831u);
  EXPECT_EQ(captured_tx->fee(), 174169u);
  EXPECT_EQ(captured_tx->invalid_after(), 155486947u);

  base::MockCallback<CardanoWalletService::SignAndPostTransactionCallback>
      post_callback;

  EXPECT_CALL(post_callback, Run(_, _, _))
      .WillOnce(RunOnceClosure(task_environment_.QuitClosure()));

  cardano_wallet_service_->SignAndPostTransaction(
      account_id(), captured_tx.value(), post_callback.Get());

  task_environment_.RunUntilQuit();
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_EQ(
      "84A400828258200100000000000000000000000000000000000000000000000000000000"
      "0000000D8258200200000000000000000000000000000000000000000000000000000000"
      "0000000D01828258390144E5E8699AB31DE351BE61DFEB7C220EFF61D29D9C88CA9D1599"
      "B36DEB20324C1F3C7C6A216E551523FF7EF4E784F3FDE3606A5BACE785391A0070EA4082"
      "58390151328FA5FAB628D3987B9CF05909A6F88C0804330747650DEE0DDF2AB7706FFA38"
      "68D70F03E0C121D82AD948F44982CC4CEFC2E9B11BB1821A0015C1A7021A0002A859031A"
      "09448AE3A1008282582039F9A9705B72246693CDACE42F68901109C805362A98038749E2"
      "FF6ECA6BEBE35840CABB91D91C5085E2247661C55879136680ACC15F51152B256EF175BC"
      "0E2A6FA8B5DE483884CA178C64FFB01CD85972CEB429FAFBD29AC5359B85E9CD01FF2A06"
      "825820D9E38698F13131246B9234BBDDE147AFBA999E34EFF03EEADDA5A336ABCA729658"
      "4084B1D0F26AD4EC27F0C1ED4189AB666F930FDB6C3CDC4901E2F9E53DC8A85125580072"
      "63A38E365467CDE45FB8E0E835C2678C429CC1F1870A4E237C2F7B6F0EF5F6",
      cardano_test_rpc_server_->captured_raw_tx());
}

}  // namespace brave_wallet
