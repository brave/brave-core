/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/components/brave_wallet/browser/tx_state_manager.h"

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_tx_meta.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class TestTxStateManagerObserver : public TxStateManager::Observer {
 public:
  TestTxStateManagerObserver() = default;

  void OnNewUnapprovedTx(mojom::TransactionInfoPtr tx) override {
    new_unapproved_tx_fired_ = true;
    tx_status_ = tx->tx_status;
    tx_id_ = tx->id;
  }

  void OnTransactionStatusChanged(mojom::TransactionInfoPtr tx) override {
    tx_status_changed_fired_ = true;
    tx_status_ = tx->tx_status;
    tx_id_ = tx->id;
  }

  void ExpectMatch(const std::string& expected_tx_id,
                   mojom::TransactionStatus expected_status) {
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(expected_tx_id, tx_id_);
    EXPECT_EQ(expected_status, tx_status_);
  }

  void Reset() {
    new_unapproved_tx_fired_ = false;
    tx_status_changed_fired_ = false;
  }

  bool NewUnapprovedTxFired() const {
    base::RunLoop().RunUntilIdle();
    return new_unapproved_tx_fired_;
  }

  bool TxStatusChangedFired() const {
    base::RunLoop().RunUntilIdle();
    return tx_status_changed_fired_;
  }

 private:
  std::string tx_id_;
  mojom::TransactionStatus tx_status_;
  bool new_unapproved_tx_fired_ = false;
  bool tx_status_changed_fired_ = false;
};

class TxStateManagerUnitTest : public testing::Test {
 public:
  TxStateManagerUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

 protected:
  void SetUp() override {
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
    json_rpc_service_ =
        std::make_unique<JsonRpcService>(shared_url_loader_factory_, &prefs_);
    // The only different between each coin type's tx state manager in these
    // base functions are their pref paths, so here we just use
    // EthTxStateManager to test common methods in TxStateManager.
    tx_state_manager_ =
        std::make_unique<EthTxStateManager>(&prefs_, json_rpc_service_.get());
  }

  void SetNetwork(const std::string& chain_id, mojom::CoinType coin) {
    base::RunLoop run_loop;
    json_rpc_service_->SetNetwork(
        chain_id, coin,
        base::BindLambdaForTesting([&](bool success) { run_loop.Quit(); }));
    run_loop.Run();
  }

  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<TxStateManager> tx_state_manager_;
};

TEST_F(TxStateManagerUnitTest, TxOperations) {
  prefs_.ClearPref(kBraveWalletTransactions);

  EthTxMeta meta;
  meta.set_id("001");
  EXPECT_FALSE(prefs_.HasPrefPath(kBraveWalletTransactions));
  // Add
  tx_state_manager_->AddOrUpdateTx(meta);
  EXPECT_TRUE(prefs_.HasPrefPath(kBraveWalletTransactions));
  {
    const auto& dict = prefs_.GetDict(kBraveWalletTransactions);
    EXPECT_EQ(dict.size(), 1u);
    const auto* ethereum_dict = dict.FindDict("ethereum");
    ASSERT_TRUE(ethereum_dict);
    EXPECT_EQ(ethereum_dict->size(), 1u);
    const auto* network_dict = ethereum_dict->FindDict("mainnet");
    ASSERT_TRUE(network_dict);
    EXPECT_EQ(network_dict->size(), 1u);
    const base::Value::Dict* value = network_dict->FindDict("001");
    ASSERT_TRUE(value);
    auto meta_from_value = tx_state_manager_->ValueToTxMeta(*value);
    ASSERT_NE(meta_from_value, nullptr);
    EXPECT_EQ(*static_cast<EthTxMeta*>(meta_from_value.get()), meta);
  }

  meta.set_tx_hash("0xabcd");
  // Update
  tx_state_manager_->AddOrUpdateTx(meta);
  {
    const auto& dict = prefs_.GetDict(kBraveWalletTransactions);
    EXPECT_EQ(dict.size(), 1u);
    const auto* ethereum_dict = dict.FindDict("ethereum");
    ASSERT_TRUE(ethereum_dict);
    EXPECT_EQ(ethereum_dict->size(), 1u);
    const auto* network_dict = ethereum_dict->FindDict("mainnet");
    ASSERT_TRUE(network_dict);
    EXPECT_EQ(network_dict->size(), 1u);
    const base::Value::Dict* value = network_dict->FindDict("001");
    ASSERT_TRUE(value);
    auto meta_from_value = tx_state_manager_->ValueToTxMeta(*value);
    ASSERT_NE(meta_from_value, nullptr);
    EXPECT_EQ(meta_from_value->tx_hash(), meta.tx_hash());
  }

  meta.set_id("002");
  meta.set_tx_hash("0xabff");
  // Add another one
  tx_state_manager_->AddOrUpdateTx(meta);
  {
    const auto& dict = prefs_.GetDict(kBraveWalletTransactions);
    EXPECT_EQ(dict.size(), 1u);
    const auto* ethereum_dict = dict.FindDict("ethereum");
    ASSERT_TRUE(ethereum_dict);
    EXPECT_EQ(ethereum_dict->size(), 1u);
    const auto* network_dict = ethereum_dict->FindDict("mainnet");
    ASSERT_TRUE(network_dict);
    EXPECT_EQ(network_dict->size(), 2u);
  }

  // Get
  {
    auto meta_fetched = tx_state_manager_->GetTx("001");
    ASSERT_NE(meta_fetched, nullptr);
    ASSERT_EQ(tx_state_manager_->GetTx("003"), nullptr);
    EXPECT_EQ(meta_fetched->id(), "001");
    EXPECT_EQ(meta_fetched->tx_hash(), "0xabcd");

    auto meta_fetched2 = tx_state_manager_->GetTx("002");
    ASSERT_NE(meta_fetched2, nullptr);
    EXPECT_EQ(meta_fetched2->id(), "002");
    EXPECT_EQ(meta_fetched2->tx_hash(), "0xabff");

    auto meta_fetched3 = tx_state_manager_->GetTx("");
    EXPECT_EQ(meta_fetched3, nullptr);
  }

  // Delete
  tx_state_manager_->DeleteTx("001");
  {
    const auto& dict = prefs_.GetDict(kBraveWalletTransactions);
    EXPECT_EQ(dict.size(), 1u);
    const auto* ethereum_dict = dict.FindDict("ethereum");
    ASSERT_TRUE(ethereum_dict);
    EXPECT_EQ(ethereum_dict->size(), 1u);
    const auto* network_dict = ethereum_dict->FindDict("mainnet");
    ASSERT_TRUE(network_dict);
    EXPECT_EQ(network_dict->size(), 1u);
  }

  // Purge
  tx_state_manager_->WipeTxs();
  EXPECT_TRUE(prefs_.HasPrefPath(kBraveWalletTransactions));
  EXPECT_FALSE(
      prefs_.HasPrefPath(std::string(kBraveWalletTransactions) + ".ethereum"));
}

TEST_F(TxStateManagerUnitTest, GetTransactionsByStatus) {
  prefs_.ClearPref(kBraveWalletTransactions);

  std::string addr1 = "0x3535353535353535353535353535353535353535";
  std::string addr2 = "0x2f015c60e0be116b1f0cd534704db9c92118fb6a";

  for (size_t i = 0; i < 20; ++i) {
    EthTxMeta meta;
    meta.set_from("0x3333333333333333333333333333333333333333");
    meta.set_id(base::NumberToString(i));
    if (i % 2 == 0) {
      if (i % 4 == 0)
        meta.set_from(addr1);
      meta.set_status(mojom::TransactionStatus::Confirmed);
    } else {
      if (i % 5 == 0)
        meta.set_from(addr2);
      meta.set_status(mojom::TransactionStatus::Submitted);
    }
    tx_state_manager_->AddOrUpdateTx(meta);
  }

  EXPECT_EQ(tx_state_manager_
                ->GetTransactionsByStatus(mojom::TransactionStatus::Approved,
                                          absl::nullopt)
                .size(),
            0u);
  EXPECT_EQ(tx_state_manager_
                ->GetTransactionsByStatus(mojom::TransactionStatus::Confirmed,
                                          absl::nullopt)
                .size(),
            10u);
  EXPECT_EQ(tx_state_manager_
                ->GetTransactionsByStatus(mojom::TransactionStatus::Submitted,
                                          absl::nullopt)
                .size(),
            10u);

  EXPECT_EQ(
      tx_state_manager_
          ->GetTransactionsByStatus(mojom::TransactionStatus::Approved, addr1)
          .size(),
      0u);

  EXPECT_EQ(
      tx_state_manager_->GetTransactionsByStatus(absl::nullopt, absl::nullopt)
          .size(),
      20u);
  EXPECT_EQ(
      tx_state_manager_->GetTransactionsByStatus(absl::nullopt, addr1).size(),
      5u);
  EXPECT_EQ(
      tx_state_manager_->GetTransactionsByStatus(absl::nullopt, addr2).size(),
      2u);

  auto confirmed_addr1 = tx_state_manager_->GetTransactionsByStatus(
      mojom::TransactionStatus::Confirmed, addr1);
  EXPECT_EQ(confirmed_addr1.size(), 5u);
  for (const auto& meta : confirmed_addr1) {
    unsigned id;
    ASSERT_TRUE(base::StringToUint(meta->id(), &id));
    EXPECT_EQ(id % 4, 0u);
  }

  auto submitted_addr2 = tx_state_manager_->GetTransactionsByStatus(
      mojom::TransactionStatus::Submitted, addr2);
  EXPECT_EQ(submitted_addr2.size(), 2u);
  for (const auto& meta : submitted_addr2) {
    unsigned id;
    ASSERT_TRUE(base::StringToUint(meta->id(), &id));
    EXPECT_EQ(id % 5, 0u);
  }
}

TEST_F(TxStateManagerUnitTest, SwitchNetwork) {
  prefs_.ClearPref(kBraveWalletTransactions);

  EthTxMeta meta;
  meta.set_id("001");
  tx_state_manager_->AddOrUpdateTx(meta);

  SetNetwork("0x5", mojom::CoinType::ETH);
  // Wait for network info
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(tx_state_manager_->GetTx("001"), nullptr);
  tx_state_manager_->AddOrUpdateTx(meta);

  SetNetwork(brave_wallet::mojom::kLocalhostChainId, mojom::CoinType::ETH);
  // Wait for network info
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(tx_state_manager_->GetTx("001"), nullptr);
  tx_state_manager_->AddOrUpdateTx(meta);

  const auto& dict = prefs_.GetDict(kBraveWalletTransactions);
  EXPECT_EQ(dict.size(), 1u);
  const auto* ethereum_dict = dict.FindDict("ethereum");
  ASSERT_TRUE(ethereum_dict);
  EXPECT_EQ(ethereum_dict->size(), 3u);
  const auto* mainnet_dict = ethereum_dict->FindDict("mainnet");
  ASSERT_TRUE(mainnet_dict);
  EXPECT_EQ(mainnet_dict->size(), 1u);
  EXPECT_TRUE(mainnet_dict->FindDict("001"));
  const auto* goerli_dict = ethereum_dict->FindDict("goerli");
  ASSERT_TRUE(goerli_dict);
  EXPECT_EQ(goerli_dict->size(), 1u);
  EXPECT_TRUE(goerli_dict->FindDict("001"));
  auto localhost_url_spec =
      brave_wallet::GetNetworkURL(&prefs_, mojom::kLocalhostChainId,
                                  mojom::CoinType::ETH)
          .spec();
  const auto* localhost_dict = ethereum_dict->FindDict(localhost_url_spec);
  ASSERT_TRUE(localhost_dict);
  EXPECT_EQ(localhost_dict->size(), 1u);
  EXPECT_TRUE(localhost_dict->FindDict("001"));
}

TEST_F(TxStateManagerUnitTest, RetireOldTxMeta) {
  prefs_.ClearPref(kBraveWalletTransactions);

  for (size_t i = 0; i < 20; ++i) {
    EthTxMeta meta;
    meta.set_id(base::NumberToString(i));
    if (i % 2 == 0) {
      meta.set_status(mojom::TransactionStatus::Confirmed);
      meta.set_confirmed_time(base::Time::Now());
    } else {
      meta.set_status(mojom::TransactionStatus::Rejected);
      meta.set_created_time(base::Time::Now());
    }
    tx_state_manager_->AddOrUpdateTx(meta);
  }

  EXPECT_TRUE(tx_state_manager_->GetTx("0"));
  EthTxMeta meta21;
  meta21.set_id("20");
  meta21.set_status(mojom::TransactionStatus::Confirmed);
  meta21.set_confirmed_time(base::Time::Now());
  tx_state_manager_->AddOrUpdateTx(meta21);
  EXPECT_FALSE(tx_state_manager_->GetTx("0"));

  EXPECT_TRUE(tx_state_manager_->GetTx("1"));
  EthTxMeta meta22;
  meta22.set_id("21");
  meta22.set_status(mojom::TransactionStatus::Rejected);
  meta22.set_created_time(base::Time::Now());
  tx_state_manager_->AddOrUpdateTx(meta22);
  EXPECT_FALSE(tx_state_manager_->GetTx("1"));

  // Other status doesn't matter
  EXPECT_TRUE(tx_state_manager_->GetTx("2"));
  EXPECT_TRUE(tx_state_manager_->GetTx("3"));
  EthTxMeta meta23;
  meta23.set_id("22");
  meta23.set_status(mojom::TransactionStatus::Submitted);
  meta23.set_created_time(base::Time::Now());
  tx_state_manager_->AddOrUpdateTx(meta23);
  EXPECT_TRUE(tx_state_manager_->GetTx("2"));
  EXPECT_TRUE(tx_state_manager_->GetTx("3"));
}

TEST_F(TxStateManagerUnitTest, Observer) {
  TestTxStateManagerObserver observer;
  tx_state_manager_->AddObserver(&observer);

  EthTxMeta meta;
  meta.set_id("001");
  // Add
  tx_state_manager_->AddOrUpdateTx(meta);
  observer.ExpectMatch("001", mojom::TransactionStatus::Unapproved);
  EXPECT_TRUE(observer.NewUnapprovedTxFired());
  EXPECT_FALSE(observer.TxStatusChangedFired());
  observer.Reset();
  // Modify
  meta.set_status(mojom::TransactionStatus::Approved);
  tx_state_manager_->AddOrUpdateTx(meta);
  observer.ExpectMatch("001", mojom::TransactionStatus::Approved);
  EXPECT_FALSE(observer.NewUnapprovedTxFired());
  EXPECT_TRUE(observer.TxStatusChangedFired());
  observer.Reset();
}

}  // namespace brave_wallet
