/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_pending_tx_tracker.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_types.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class EthPendingTxTrackerUnitTest : public testing::Test {
 public:
  EthPendingTxTrackerUnitTest() {
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
  }

  void SetUp() override {
    TestingProfile::Builder builder;
    auto prefs =
        std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
    RegisterUserProfilePrefs(prefs->registry());
    builder.SetPrefService(std::move(prefs));
    profile_ = builder.Build();
  }

  PrefService* GetPrefs() { return profile_->GetPrefs(); }

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
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(EthPendingTxTrackerUnitTest, IsNonceTaken) {
  EthJsonRpcController controller(shared_url_loader_factory(), GetPrefs());
  EthTxStateManager tx_state_manager(GetPrefs(), controller.MakeRemote());
  EthNonceTracker nonce_tracker(&tx_state_manager, &controller);
  EthPendingTxTracker pending_tx_tracker(&tx_state_manager, &controller,
                                         &nonce_tracker);

  EthTxStateManager::TxMeta meta;
  meta.from = EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  meta.id = EthTxStateManager::GenerateMetaID();
  meta.tx->set_nonce(uint256_t(123));

  EXPECT_FALSE(pending_tx_tracker.IsNonceTaken(meta));

  EthTxStateManager::TxMeta meta_in_state;
  meta_in_state.id = EthTxStateManager::GenerateMetaID();
  meta_in_state.status = mojom::TransactionStatus::Confirmed;
  meta_in_state.from = meta.from;
  meta_in_state.tx->set_nonce(uint256_t(123));
  tx_state_manager.AddOrUpdateTx(meta_in_state);

  EXPECT_TRUE(pending_tx_tracker.IsNonceTaken(meta));
}

TEST_F(EthPendingTxTrackerUnitTest, ShouldTxDropped) {
  EthAddress addr =
      EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  EthJsonRpcController controller(shared_url_loader_factory(), GetPrefs());
  EthTxStateManager tx_state_manager(GetPrefs(), controller.MakeRemote());
  EthNonceTracker nonce_tracker(&tx_state_manager, &controller);
  EthPendingTxTracker pending_tx_tracker(&tx_state_manager, &controller,
                                         &nonce_tracker);
  pending_tx_tracker.network_nonce_map_[addr.ToHex()] = uint256_t(3);

  EthTxStateManager::TxMeta meta;
  meta.from = addr;
  meta.id = EthTxStateManager::GenerateMetaID();
  meta.tx_hash =
      "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238";
  meta.tx->set_nonce(uint256_t(1));
  EXPECT_TRUE(pending_tx_tracker.ShouldTxDropped(meta));
  EXPECT_TRUE(pending_tx_tracker.network_nonce_map_.find(addr.ToHex()) ==
              pending_tx_tracker.network_nonce_map_.end());

  meta.tx->set_nonce(uint256_t(4));
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
  EthJsonRpcController controller(shared_url_loader_factory(), GetPrefs());
  EthTxStateManager tx_state_manager(GetPrefs(), controller.MakeRemote());
  EthNonceTracker nonce_tracker(&tx_state_manager, &controller);
  EthPendingTxTracker pending_tx_tracker(&tx_state_manager, &controller,
                                         &nonce_tracker);
  EthTxStateManager::TxMeta meta;
  meta.id = "001";
  meta.status = mojom::TransactionStatus::Submitted;
  tx_state_manager.AddOrUpdateTx(meta);

  pending_tx_tracker.DropTransaction(&meta);
  EXPECT_EQ(tx_state_manager.GetTx("001"), nullptr);
}

TEST_F(EthPendingTxTrackerUnitTest, UpdatePendingTransactions) {
  EthAddress addr1 =
      EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  EthAddress addr2 =
      EthAddress::FromHex("0x2f015c60e0be116b1f0cd534704db9c92118fb6b");
  EthJsonRpcController controller(shared_url_loader_factory(), GetPrefs());
  EthTxStateManager tx_state_manager(GetPrefs(), controller.MakeRemote());
  EthNonceTracker nonce_tracker(&tx_state_manager, &controller);
  EthPendingTxTracker pending_tx_tracker(&tx_state_manager, &controller,
                                         &nonce_tracker);
  base::RunLoop().RunUntilIdle();
  EthTxStateManager::TxMeta meta;
  meta.id = "001";
  meta.from = addr1;
  meta.status = mojom::TransactionStatus::Submitted;
  tx_state_manager.AddOrUpdateTx(meta);
  meta.id = "002";
  meta.from = addr2;
  meta.tx->set_nonce(uint256_t(4));
  meta.status = mojom::TransactionStatus::Confirmed;
  tx_state_manager.AddOrUpdateTx(meta);
  meta.id = "003";
  meta.from = addr2;
  meta.tx->set_nonce(uint256_t(4));
  meta.status = mojom::TransactionStatus::Submitted;
  tx_state_manager.AddOrUpdateTx(meta);
  meta.id = "004";
  meta.from = addr2;
  meta.tx->set_nonce(uint256_t(5));
  meta.status = mojom::TransactionStatus::Submitted;
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
  auto meta_from_state = tx_state_manager.GetTx("001");
  ASSERT_NE(meta_from_state, nullptr);
  EXPECT_EQ(meta_from_state->status, mojom::TransactionStatus::Confirmed);
  EXPECT_EQ(meta_from_state->from, addr1);
  EXPECT_EQ(meta_from_state->tx_receipt.contract_address,
            "0xb60e8dd61c5d32be8058bb8eb970870f07233155");

  meta_from_state = tx_state_manager.GetTx("003");
  ASSERT_EQ(meta_from_state, nullptr);
  meta_from_state = tx_state_manager.GetTx("004");
  ASSERT_NE(meta_from_state, nullptr);
  EXPECT_EQ(meta_from_state->status, mojom::TransactionStatus::Confirmed);
  EXPECT_EQ(meta_from_state->tx_receipt.contract_address,
            "0xb60e8dd61c5d32be8058bb8eb970870f07233155");
}

}  // namespace brave_wallet
