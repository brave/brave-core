/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_tx_manager.h"

#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_test_utils.h"
#include "brave/components/brave_wallet/browser/brave_wallet_p3a.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom-forward.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

using testing::_;
using testing::SaveArg;

namespace brave_wallet {

class BitcoinTxManagerUnitTest : public testing::Test {
 public:
  BitcoinTxManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  void SetUp() override {
    brave_wallet::RegisterLocalStatePrefs(local_state_.registry());
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    brave_wallet::RegisterProfilePrefsForMigration(prefs_.registry());
    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    json_rpc_service_ = std::make_unique<JsonRpcService>(
        shared_url_loader_factory_, network_manager_.get(), &prefs_, nullptr);
    keyring_service_ = std::make_unique<KeyringService>(json_rpc_service_.get(),
                                                        &prefs_, &local_state_);

    bitcoin_test_rpc_server_ = std::make_unique<BitcoinTestRpcServer>();
    bitcoin_wallet_service_ = std::make_unique<BitcoinWalletService>(
        *keyring_service_, *network_manager_,
        bitcoin_test_rpc_server_->GetURLLoaderFactory());
    bitcoin_wallet_service_->SetArrangeTransactionsForTesting(true);

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    tx_service_ = std::make_unique<TxService>(
        json_rpc_service_.get(), bitcoin_wallet_service_.get(), nullptr,
        nullptr, *keyring_service_, &prefs_, temp_dir_.GetPath(),
        base::SequencedTaskRunner::GetCurrentDefault());
    WaitForTxStorageDelegateInitialized(tx_service_->GetDelegateForTesting());

    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, "brave");

    task_environment_.RunUntilIdle();

    bitcoin_test_rpc_server_->SetUpBitcoinRpc(kMnemonicDivideCruise, 0);
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  mojom::AccountIdPtr BtcAcc(size_t index) {
    return GetAccountUtils().EnsureBtcAccount(index)->account_id->Clone();
  }

  BitcoinTxManager* btc_tx_manager() {
    return tx_service_->GetBitcoinTxManager();
  }

  PrefService* prefs() { return &prefs_; }

  void AddUnapprovedTransaction(
      std::string chain_id,
      mojom::TxDataUnionPtr tx_data_union,
      mojom::AccountIdPtr from,
      std::optional<url::Origin> origin,
      BitcoinTxManager::AddUnapprovedTransactionCallback callback) {
    btc_tx_manager()->AddUnapprovedTransaction(
        std::move(chain_id), std::move(tx_data_union), std::move(from),
        std::move(origin), std::move(callback));
  }

  void ApproveTransaction(
      std::string tx_meta_id,
      BitcoinTxManager::ApproveTransactionCallback callback) {
    btc_tx_manager()->ApproveTransaction(std::move(tx_meta_id),
                                         std::move(callback));
  }

 protected:
  base::test::ScopedFeatureList feature_list_{
      features::kBraveWalletBitcoinFeature};
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<BitcoinTestRpcServer> bitcoin_test_rpc_server_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<BitcoinWalletService> bitcoin_wallet_service_;
  std::unique_ptr<TxService> tx_service_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(BitcoinTxManagerUnitTest, SubmitTransaction) {
  const auto from_account = BtcAcc(0);
  std::string to_account = kMockBtcAddress;
  auto params = mojom::NewBitcoinTransactionParams::New(
      mojom::kBitcoinMainnet, from_account.Clone(), kMockBtcAddress, 5000,
      false);

  base::MockCallback<TxManager::AddUnapprovedTransactionCallback> add_callback;
  std::string meta_id;
  EXPECT_CALL(add_callback, Run(_, _, _)).WillOnce(SaveArg<1>(&meta_id));
  btc_tx_manager()->AddUnapprovedBitcoinTransaction(params.Clone(),
                                                    add_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&add_callback);
  EXPECT_TRUE(!meta_id.empty());

  auto tx_meta1 = btc_tx_manager()->GetTxForTesting(meta_id);
  EXPECT_TRUE(tx_meta1);
  EXPECT_EQ(tx_meta1->chain_id(), mojom::kBitcoinMainnet);

  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Unapproved);

  base::MockCallback<TxManager::ApproveTransactionCallback> approve_callback;
  EXPECT_CALL(approve_callback, Run(true, _, _));
  ApproveTransaction(meta_id, approve_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&approve_callback);

  tx_meta1 = btc_tx_manager()->GetTxForTesting(meta_id);
  ASSERT_TRUE(tx_meta1);
  EXPECT_FALSE(tx_meta1->tx_hash().empty());
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Submitted);

  bitcoin_test_rpc_server_->ConfirmAllTransactions();
  btc_tx_manager()->UpdatePendingTransactions(mojom::kBitcoinMainnet);
  task_environment_.RunUntilIdle();
  tx_meta1 = btc_tx_manager()->GetTxForTesting(meta_id);
  ASSERT_TRUE(tx_meta1);
  EXPECT_FALSE(tx_meta1->tx_hash().empty());
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Confirmed);
}

TEST_F(BitcoinTxManagerUnitTest, SubmitTransactionError) {
  const auto from_account = BtcAcc(0);
  std::string to_account = kMockBtcAddress;

  auto params = mojom::NewBitcoinTransactionParams::New(
      mojom::kBitcoinMainnet, from_account.Clone(), kMockBtcAddress, 5000,
      false);

  base::MockCallback<TxManager::AddUnapprovedTransactionCallback> add_callback;
  std::string meta_id;
  EXPECT_CALL(add_callback, Run(_, _, _)).WillOnce(SaveArg<1>(&meta_id));
  btc_tx_manager()->AddUnapprovedBitcoinTransaction(params.Clone(),
                                                    add_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&add_callback);
  EXPECT_TRUE(!meta_id.empty());

  auto tx_meta1 = btc_tx_manager()->GetTxForTesting(meta_id);
  EXPECT_TRUE(tx_meta1);
  EXPECT_EQ(tx_meta1->chain_id(), mojom::kBitcoinMainnet);

  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Unapproved);

  bitcoin_test_rpc_server_->FailNextTransactionBroadcast();

  base::MockCallback<TxManager::ApproveTransactionCallback> approve_callback;
  EXPECT_CALL(approve_callback, Run(false, _, _));
  btc_tx_manager()->ApproveTransaction(meta_id, approve_callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&approve_callback);

  tx_meta1 = btc_tx_manager()->GetTxForTesting(meta_id);
  ASSERT_TRUE(tx_meta1);
  EXPECT_TRUE(tx_meta1->tx_hash().empty());
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Error);
}

TEST_F(BitcoinTxManagerUnitTest, MojoRejectsInvalidBitcoinTransactionParams) {
  mojo::Remote<mojom::TxService> tx_service_remote;
  tx_service_->Bind(tx_service_remote.BindNewPipeAndPassReceiver());
  ASSERT_TRUE(tx_service_remote.is_connected());

  const auto valid_account = BtcAcc(0);
  const std::string valid_address = kMockBtcAddress;

  auto make_params = [&](mojom::AccountIdPtr from, const std::string& to,
                         uint64_t amount) {
    return mojom::NewBitcoinTransactionParams::New(
        mojom::kBitcoinMainnet, std::move(from), to, amount,
        /*sending_max_amount=*/false);
  };

  auto call =
      [this, &tx_service_remote](mojom::NewBitcoinTransactionParamsPtr params) {
        base::MockCallback<
            mojom::TxService::AddUnapprovedBitcoinTransactionCallback>
            callback;
        bool success = true;
        std::string meta_id;
        std::string error_message;
        EXPECT_CALL(callback, Run(_, _, _))
            .WillOnce([&](bool success_in, const std::string& meta_in,
                          const std::string& error_in) {
              success = success_in;
              meta_id = meta_in;
              error_message = error_in;
            });

        tx_service_remote->AddUnapprovedBitcoinTransaction(std::move(params),
                                                           callback.Get());
        tx_service_remote.FlushForTesting();
        task_environment_.RunUntilIdle();
        testing::Mock::VerifyAndClearExpectations(&callback);
        return std::make_tuple(success, meta_id, error_message);
      };

  // Oversized destination address should fail fast and keep the pipe alive.
  const std::string oversized_address(4096u, 'a');
  auto [oversized_success, oversized_meta, oversized_error] =
      call(make_params(valid_account->Clone(), oversized_address, 5000u));
  EXPECT_FALSE(oversized_success);
  EXPECT_TRUE(oversized_meta.empty());
  EXPECT_FALSE(oversized_error.empty());
  EXPECT_TRUE(tx_service_remote.is_connected());

  // Zero-amount transactions are rejected before any mutation occurs.
  auto [zero_success, zero_meta, zero_error] =
      call(make_params(valid_account->Clone(), valid_address, 0u));
  EXPECT_FALSE(zero_success);
  EXPECT_TRUE(zero_meta.empty());
  EXPECT_FALSE(zero_error.empty());

  // Non-existent account IDs should be rejected without crashing.
  auto invalid_account =
      mojom::AccountId::New(mojom::CoinType::BTC, mojom::KeyringId::kBitcoin84,
                            mojom::AccountKind::kDerived, valid_address,
                            /*account_index=*/999u,
                            /*unique_key=*/"");
  auto [invalid_success, invalid_meta, invalid_error] =
      call(make_params(std::move(invalid_account), valid_address, 5000u));
  EXPECT_FALSE(invalid_success);
  EXPECT_TRUE(invalid_meta.empty());
  EXPECT_FALSE(invalid_error.empty());

  // Extremely large amounts should surface an error, not a crash.
  auto [overflow_success, overflow_meta, overflow_error] =
      call(make_params(valid_account->Clone(), valid_address,
                       std::numeric_limits<uint64_t>::max()));
  EXPECT_FALSE(overflow_success);
  EXPECT_TRUE(overflow_meta.empty());
  EXPECT_FALSE(overflow_error.empty());

  EXPECT_TRUE(tx_service_remote.is_connected());
}

}  //  namespace brave_wallet
