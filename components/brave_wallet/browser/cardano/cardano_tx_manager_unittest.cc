/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_tx_manager.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/files/scoped_temp_dir.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_p3a.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_test_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

using base::test::RunOnceClosure;
using testing::_;
using testing::SaveArg;

namespace brave_wallet {

namespace {
class MockTxStateManagerObserver : public TxStateManager::Observer {
 public:
  explicit MockTxStateManagerObserver(TxStateManager& tx_state_manager) {
    observation_.Observe(&tx_state_manager);
  }

  MOCK_METHOD1(OnTransactionStatusChanged, void(mojom::TransactionInfoPtr));
  MOCK_METHOD1(OnNewUnapprovedTx, void(mojom::TransactionInfoPtr));

 private:
  base::ScopedObservation<TxStateManager, TxStateManager::Observer>
      observation_{this};
};
}  // namespace

class CardanoTxManagerUnitTest : public testing::Test {
 public:
  CardanoTxManagerUnitTest()
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

    cardano_wallet_service_ = std::make_unique<CardanoWalletService>(
        *keyring_service_, *network_manager_,
        scoped_refptr<network::SharedURLLoaderFactory>());
    cardano_test_rpc_server_ =
        std::make_unique<CardanoTestRpcServer>(*cardano_wallet_service_);

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    tx_service_ = std::make_unique<TxService>(
        json_rpc_service_.get(), nullptr, nullptr,
        cardano_wallet_service_.get(), *keyring_service_, &prefs_,
        temp_dir_.GetPath(), base::SequencedTaskRunner::GetCurrentDefault());
    WaitForTxStorageDelegateInitialized(tx_service_->GetDelegateForTesting());

    GetAccountUtils().CreateWallet(kMnemonicDivideCruise, "brave");

    cardano_test_rpc_server_->SetUpCardanoRpc(kMnemonicDivideCruise, 0);
  }

  AccountUtils GetAccountUtils() {
    return AccountUtils(keyring_service_.get());
  }

  mojom::AccountIdPtr CardanoAcc(size_t index) {
    return GetAccountUtils().EnsureAdaAccount(index)->account_id->Clone();
  }

  CardanoTxManager* cardano_tx_manager() {
    return tx_service_->GetCardanoTxManager();
  }

  PrefService* prefs() { return &prefs_; }

  void AddUnapprovedTransaction(
      std::string chain_id,
      mojom::TxDataUnionPtr tx_data_union,
      mojom::AccountIdPtr from,
      std::optional<url::Origin> origin,
      CardanoTxManager::AddUnapprovedTransactionCallback callback) {
    cardano_tx_manager()->AddUnapprovedTransaction(
        std::move(chain_id), std::move(tx_data_union), std::move(from),
        std::move(origin), std::move(callback));
  }

  void ApproveTransaction(
      std::string tx_meta_id,
      CardanoTxManager::ApproveTransactionCallback callback) {
    cardano_tx_manager()->ApproveTransaction(std::move(tx_meta_id),
                                             std::move(callback));
  }

 protected:
  base::test::ScopedFeatureList feature_list_{
      features::kBraveWalletCardanoFeature};
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<CardanoWalletService> cardano_wallet_service_;
  std::unique_ptr<CardanoTestRpcServer> cardano_test_rpc_server_;
  std::unique_ptr<TxService> tx_service_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(CardanoTxManagerUnitTest, SubmitTransaction) {
  const auto from_account = CardanoAcc(0);
  std::string to_account = kMockCardanoAddress1;
  auto params = mojom::NewCardanoTransactionParams::New(
      mojom::kCardanoMainnet, from_account.Clone(), kMockCardanoAddress1,
      1000000, false);

  base::MockCallback<TxManager::AddUnapprovedTransactionCallback> add_callback;
  std::string meta_id;
  EXPECT_CALL(add_callback, Run(_, _, _))
      .WillOnce(
          testing::DoAll(SaveArg<1>(&meta_id),
                         RunOnceClosure(task_environment_.QuitClosure())));
  cardano_tx_manager()->AddUnapprovedCardanoTransaction(params.Clone(),
                                                        add_callback.Get());
  task_environment_.RunUntilQuit();
  testing::Mock::VerifyAndClearExpectations(&add_callback);
  EXPECT_TRUE(!meta_id.empty());

  auto tx_meta1 = cardano_tx_manager()->GetTxForTesting(meta_id);
  EXPECT_TRUE(tx_meta1);
  EXPECT_EQ(tx_meta1->chain_id(), mojom::kCardanoMainnet);

  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Unapproved);

  base::MockCallback<TxManager::ApproveTransactionCallback> approve_callback;
  EXPECT_CALL(approve_callback, Run(true, _, _))
      .WillOnce(RunOnceClosure(task_environment_.QuitClosure()));
  ApproveTransaction(meta_id, approve_callback.Get());
  task_environment_.RunUntilQuit();
  testing::Mock::VerifyAndClearExpectations(&approve_callback);

  tx_meta1 = cardano_tx_manager()->GetTxForTesting(meta_id);
  ASSERT_TRUE(tx_meta1);
  EXPECT_FALSE(tx_meta1->tx_hash().empty());
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Submitted);

  MockTxStateManagerObserver observer(
      cardano_tx_manager()->GetCardanoTxStateManager());

  cardano_test_rpc_server_->ConfirmAllTransactions();
  EXPECT_CALL(observer, OnTransactionStatusChanged(_))
      .Times(1)
      .WillOnce(RunOnceClosure(task_environment_.QuitClosure()));
  cardano_tx_manager()->UpdatePendingTransactions(mojom::kCardanoMainnet);
  task_environment_.RunUntilQuit();
  tx_meta1 = cardano_tx_manager()->GetTxForTesting(meta_id);
  ASSERT_TRUE(tx_meta1);
  EXPECT_FALSE(tx_meta1->tx_hash().empty());
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Confirmed);
}

TEST_F(CardanoTxManagerUnitTest, SubmitTransactionError) {
  const auto from_account = CardanoAcc(0);
  std::string to_account = kMockCardanoAddress1;

  auto params = mojom::NewCardanoTransactionParams::New(
      mojom::kCardanoMainnet, from_account.Clone(), kMockCardanoAddress1,
      1000000, false);

  base::MockCallback<TxManager::AddUnapprovedTransactionCallback> add_callback;
  std::string meta_id;
  EXPECT_CALL(add_callback, Run(_, _, _))
      .WillOnce(
          testing::DoAll(SaveArg<1>(&meta_id),
                         RunOnceClosure(task_environment_.QuitClosure())));
  cardano_tx_manager()->AddUnapprovedCardanoTransaction(params.Clone(),
                                                        add_callback.Get());
  task_environment_.RunUntilQuit();
  testing::Mock::VerifyAndClearExpectations(&add_callback);
  EXPECT_TRUE(!meta_id.empty());

  auto tx_meta1 = cardano_tx_manager()->GetTxForTesting(meta_id);
  EXPECT_TRUE(tx_meta1);
  EXPECT_EQ(tx_meta1->chain_id(), mojom::kCardanoMainnet);

  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Unapproved);

  cardano_test_rpc_server_->FailNextTransactionSubmission();

  base::MockCallback<TxManager::ApproveTransactionCallback> approve_callback;
  EXPECT_CALL(approve_callback, Run(false, _, _))
      .WillOnce(RunOnceClosure(task_environment_.QuitClosure()));
  cardano_tx_manager()->ApproveTransaction(meta_id, approve_callback.Get());
  task_environment_.RunUntilQuit();
  testing::Mock::VerifyAndClearExpectations(&approve_callback);

  tx_meta1 = cardano_tx_manager()->GetTxForTesting(meta_id);
  ASSERT_TRUE(tx_meta1);
  EXPECT_TRUE(tx_meta1->tx_hash().empty());
  EXPECT_EQ(tx_meta1->from(), from_account);
  EXPECT_EQ(tx_meta1->status(), mojom::TransactionStatus::Error);
}

}  //  namespace brave_wallet
