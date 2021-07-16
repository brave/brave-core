/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_pending_tx_tracker.h"

#include "base/bind.h"
#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_types.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class EthPendingTxTrackerUnitTest : public testing::Test {
 public:
  EthPendingTxTrackerUnitTest()
      : testing_profile_manager_(TestingBrowserProcess::GetGlobal()) {
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
  }

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(testing_profile_manager_.SetUp(temp_dir_.GetPath()));
  }

  PrefService* GetPrefs() {
    return ProfileManager::GetActiveUserProfile()->GetPrefs();
  }

  network::SharedURLLoaderFactory* shared_url_loader_factory() {
    return url_loader_factory_.GetSafeWeakWrapper().get();
  }

  network::TestURLLoaderFactory* test_url_loader_factory() {
    return &url_loader_factory_;
  }

  void WaitForResponse() { task_environment_.RunUntilIdle(); }

 private:
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  content::BrowserTaskEnvironment task_environment_;
  TestingProfileManager testing_profile_manager_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(EthPendingTxTrackerUnitTest, IsNonceTaken) {
  EthJsonRpcController controller(Network::kMainnet,
                                  shared_url_loader_factory());
  EthTxStateManager tx_state_manager(GetPrefs());
  EthNonceTracker nonce_tracker(&tx_state_manager, &controller);
  EthPendingTxTracker pending_tx_tracker(&tx_state_manager, &controller,
                                         &nonce_tracker);

  EthTxStateManager::TxMeta meta;
  meta.from = EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  meta.id = EthTxStateManager::GenerateMetaID();
  meta.tx.set_nonce(uint256_t(123));

  EXPECT_FALSE(pending_tx_tracker.IsNonceTaken(meta));

  EthTxStateManager::TxMeta meta_in_state(meta);
  meta_in_state.id = EthTxStateManager::GenerateMetaID();
  meta_in_state.status = EthTxStateManager::TransactionStatus::CONFIRMED;
  tx_state_manager.AddOrUpdateTx(meta_in_state);

  EXPECT_TRUE(pending_tx_tracker.IsNonceTaken(meta));
}

TEST_F(EthPendingTxTrackerUnitTest, ShouldTxDropped) {
  EthAddress addr =
      EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  EthJsonRpcController controller(Network::kMainnet,
                                  shared_url_loader_factory());
  EthTxStateManager tx_state_manager(GetPrefs());
  EthNonceTracker nonce_tracker(&tx_state_manager, &controller);
  EthPendingTxTracker pending_tx_tracker(&tx_state_manager, &controller,
                                         &nonce_tracker);
  pending_tx_tracker.network_nonce_map_[addr.ToHex()] = uint256_t(3);

  EthTxStateManager::TxMeta meta;
  meta.from = addr;
  meta.id = EthTxStateManager::GenerateMetaID();
  meta.tx_hash =
      "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238";
  meta.tx.set_nonce(uint256_t(1));
  EXPECT_TRUE(pending_tx_tracker.ShouldTxDropped(meta));
  EXPECT_TRUE(pending_tx_tracker.network_nonce_map_.find(addr.ToHex()) ==
              pending_tx_tracker.network_nonce_map_.end());

  meta.tx.set_nonce(uint256_t(4));
  EXPECT_FALSE(pending_tx_tracker.ShouldTxDropped(meta));
  EXPECT_FALSE(pending_tx_tracker.ShouldTxDropped(meta));
  EXPECT_FALSE(pending_tx_tracker.ShouldTxDropped(meta));
  // drop
  EXPECT_EQ(pending_tx_tracker.dropped_blocks_counter_[meta.tx_hash], 3);
  EXPECT_TRUE(pending_tx_tracker.ShouldTxDropped(meta));
  EXPECT_TRUE(pending_tx_tracker.dropped_blocks_counter_.find(meta.tx_hash) ==
              pending_tx_tracker.dropped_blocks_counter_.end());
}

TEST_F(EthPendingTxTrackerUnitTest, DropTransaction) {
  EthJsonRpcController controller(Network::kMainnet,
                                  shared_url_loader_factory());
  EthTxStateManager tx_state_manager(GetPrefs());
  EthNonceTracker nonce_tracker(&tx_state_manager, &controller);
  EthPendingTxTracker pending_tx_tracker(&tx_state_manager, &controller,
                                         &nonce_tracker);
  EthTxStateManager::TxMeta meta;
  meta.id = "001";
  meta.status = EthTxStateManager::TransactionStatus::SUBMITTED;
  tx_state_manager.AddOrUpdateTx(meta);

  pending_tx_tracker.DropTransaction(meta);
  EthTxStateManager::TxMeta meta_from_state;
  ASSERT_TRUE(tx_state_manager.GetTx("001", &meta_from_state));
  EXPECT_EQ(meta_from_state.status,
            EthTxStateManager::TransactionStatus::DROPPED);
}

TEST_F(EthPendingTxTrackerUnitTest, UpdatePendingTransactions) {
  EthAddress addr1 =
      EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  EthAddress addr2 =
      EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6b");
  EthJsonRpcController controller(Network::kMainnet,
                                  shared_url_loader_factory());
  EthTxStateManager tx_state_manager(GetPrefs());
  EthNonceTracker nonce_tracker(&tx_state_manager, &controller);
  EthPendingTxTracker pending_tx_tracker(&tx_state_manager, &controller,
                                         &nonce_tracker);
  EthTxStateManager::TxMeta meta;
  meta.id = "001";
  meta.from = addr1;
  meta.status = EthTxStateManager::TransactionStatus::SUBMITTED;
  tx_state_manager.AddOrUpdateTx(meta);
  meta.id = "002";
  meta.from = addr2;
  meta.tx.set_nonce(uint256_t(4));
  meta.status = EthTxStateManager::TransactionStatus::CONFIRMED;
  tx_state_manager.AddOrUpdateTx(meta);
  meta.id = "003";
  meta.from = addr2;
  meta.tx.set_nonce(uint256_t(4));
  meta.status = EthTxStateManager::TransactionStatus::SUBMITTED;
  tx_state_manager.AddOrUpdateTx(meta);
  meta.id = "004";
  meta.from = addr2;
  meta.tx.set_nonce(uint256_t(5));
  meta.status = EthTxStateManager::TransactionStatus::SUBMITTED;
  tx_state_manager.AddOrUpdateTx(meta);

  test_url_loader_factory()->SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        test_url_loader_factory()->AddResponse(
            request.url.spec(),
            "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{"
            "\"transactionHash\":"
            "\"0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce5682"
            "38\","
            "\"transactionIndex\":  \"0x1\","
            "\"blockNumber\": \"0xb\","
            "\"blockHash\": "
            "\"0xc6ef2fc5426d6ad6fd9e2a26abeab0aa2411b7ab17f30a99d3cb96aed1d105"
            "5b\","
            "\"cumulativeGasUsed\": \"0x33bc\","
            "\"gasUsed\": \"0x4dc\","
            "\"contractAddress\": "
            "\"0xb60e8dd61c5d32be8058bb8eb970870f07233155\","
            "\"logs\": [],"
            "\"logsBloom\": \"0x00...0\","
            "\"status\": \"0x1\"}}");
      }));

  pending_tx_tracker.UpdatePendingTransactions();
  WaitForResponse();
  EthTxStateManager::TxMeta meta_from_state;
  ASSERT_TRUE(tx_state_manager.GetTx("001", &meta_from_state));
  EXPECT_EQ(meta_from_state.status,
            EthTxStateManager::TransactionStatus::CONFIRMED);
  EXPECT_EQ(meta_from_state.from, addr1);
  EXPECT_EQ(meta_from_state.tx_receipt.contract_address,
            "0xb60e8dd61c5d32be8058bb8eb970870f07233155");

  ASSERT_TRUE(tx_state_manager.GetTx("003", &meta_from_state));
  EXPECT_EQ(meta_from_state.from, addr2);
  EXPECT_EQ(meta_from_state.status,
            EthTxStateManager::TransactionStatus::DROPPED);
  EXPECT_NE(meta_from_state.tx_receipt.contract_address,
            "0xb60e8dd61c5d32be8058bb8eb970870f07233155");
  ASSERT_TRUE(tx_state_manager.GetTx("004", &meta_from_state));
  EXPECT_EQ(meta_from_state.status,
            EthTxStateManager::TransactionStatus::CONFIRMED);
  EXPECT_EQ(meta_from_state.tx_receipt.contract_address,
            "0xb60e8dd61c5d32be8058bb8eb970870f07233155");
}

}  // namespace brave_wallet
