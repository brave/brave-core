/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_pending_tx_tracker.h"

#include <memory>
#include <utility>

#include "base/containers/contains.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_meta.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_storage_delegate_impl.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
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
    factory_ = GetTestValueStoreFactory(temp_dir_);
    delegate_ = GetTxStorageDelegateForTest(GetPrefs(), factory_);
    account_resolver_delegate_ =
        std::make_unique<AccountResolverDelegateForTest>();
    tx_state_manager_ = std::make_unique<EthTxStateManager>(
        *delegate_, *account_resolver_delegate_);

    eth_account_id_ = account_resolver_delegate_->RegisterAccount(
        MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                      mojom::AccountKind::kDerived,
                      "0x2f015c60e0be116b1f0cd534704db9c92118fb6a"));
    eth_account_id_other_ = account_resolver_delegate_->RegisterAccount(
        MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                      mojom::AccountKind::kDerived,
                      "0x2f015c60e0be116b1f0cd534704db9c92118fb6b"));

    network_manager_ = std::make_unique<NetworkManager>(GetPrefs());
    json_rpc_service_ = std::make_unique<JsonRpcService>(
        shared_url_loader_factory(), network_manager_.get(), GetPrefs(),
        nullptr);
  }

  JsonRpcService* json_rpc_service() { return json_rpc_service_.get(); }

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
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  base::ScopedTempDir temp_dir_;
  scoped_refptr<value_store::TestValueStoreFactory> factory_;
  std::unique_ptr<value_store::ValueStoreFrontend> storage_;
  std::unique_ptr<TxStorageDelegateImpl> delegate_;
  std::unique_ptr<AccountResolverDelegateForTest> account_resolver_delegate_;
  std::unique_ptr<TestingProfile> profile_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;

 protected:
  std::unique_ptr<EthTxStateManager> tx_state_manager_;
  mojom::AccountIdPtr eth_account_id_;
  mojom::AccountIdPtr eth_account_id_other_;
};

TEST_F(EthPendingTxTrackerUnitTest, IsNonceTaken) {
  EthNonceTracker nonce_tracker(tx_state_manager_.get(), json_rpc_service());
  EthPendingTxTracker pending_tx_tracker(tx_state_manager_.get(),
                                         json_rpc_service(), &nonce_tracker);

  EthTxMeta meta(eth_account_id_, std::make_unique<EthTransaction>());
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_chain_id(mojom::kMainnetChainId);
  meta.tx()->set_nonce(uint256_t(123));

  EXPECT_FALSE(pending_tx_tracker.IsNonceTaken(meta));

  EthTxMeta meta_in_state(meta.from(), std::make_unique<EthTransaction>());
  meta_in_state.set_id(TxMeta::GenerateMetaID());
  meta_in_state.set_chain_id(meta.chain_id());
  meta_in_state.set_status(mojom::TransactionStatus::Confirmed);
  meta_in_state.tx()->set_nonce(meta.tx()->nonce());
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta_in_state));

  EXPECT_TRUE(pending_tx_tracker.IsNonceTaken(meta));

  meta.set_chain_id(mojom::kSepoliaChainId);
  EXPECT_FALSE(pending_tx_tracker.IsNonceTaken(meta));
}

TEST_F(EthPendingTxTrackerUnitTest, ShouldTxDropped) {
  std::string addr = eth_account_id_->address;
  EthNonceTracker nonce_tracker(tx_state_manager_.get(), json_rpc_service());
  EthPendingTxTracker pending_tx_tracker(tx_state_manager_.get(),
                                         json_rpc_service(), &nonce_tracker);
  pending_tx_tracker.network_nonce_map_[addr][mojom::kMainnetChainId] =
      uint256_t(3);

  EthTxMeta meta(eth_account_id_, std::make_unique<EthTransaction>());
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_chain_id(mojom::kMainnetChainId);
  meta.set_tx_hash(
      "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238");
  meta.tx()->set_nonce(uint256_t(1));
  EXPECT_TRUE(pending_tx_tracker.ShouldTxDropped(meta));
  EXPECT_FALSE(base::Contains(pending_tx_tracker.network_nonce_map_, addr));

  meta.tx()->set_nonce(uint256_t(4));
  EXPECT_FALSE(pending_tx_tracker.ShouldTxDropped(meta));
  EXPECT_FALSE(pending_tx_tracker.ShouldTxDropped(meta));
  EXPECT_FALSE(pending_tx_tracker.ShouldTxDropped(meta));
  // drop
  EXPECT_EQ(pending_tx_tracker.dropped_blocks_counter_[meta.tx_hash()], 3);
  EXPECT_TRUE(pending_tx_tracker.ShouldTxDropped(meta));
  EXPECT_FALSE(base::Contains(pending_tx_tracker.dropped_blocks_counter_,
                              (meta.tx_hash())));
}

TEST_F(EthPendingTxTrackerUnitTest, DropTransaction) {
  EthNonceTracker nonce_tracker(tx_state_manager_.get(), json_rpc_service());
  EthPendingTxTracker pending_tx_tracker(tx_state_manager_.get(),
                                         json_rpc_service(), &nonce_tracker);
  EthTxMeta meta(eth_account_id_, std::make_unique<EthTransaction>());
  meta.set_id("001");
  meta.set_chain_id(mojom::kMainnetChainId);
  meta.set_status(mojom::TransactionStatus::Submitted);
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));

  pending_tx_tracker.DropTransaction(&meta);
  EXPECT_EQ(tx_state_manager_->GetTx("001"), nullptr);
}

TEST_F(EthPendingTxTrackerUnitTest, UpdatePendingTransactions) {
  EthNonceTracker nonce_tracker(tx_state_manager_.get(), json_rpc_service());
  EthPendingTxTracker pending_tx_tracker(tx_state_manager_.get(),
                                         json_rpc_service(), &nonce_tracker);
  base::RunLoop().RunUntilIdle();

  for (const std::string& chain_id :
       {mojom::kMainnetChainId, mojom::kSepoliaChainId,
        mojom::kSepoliaChainId}) {
    EthTxMeta meta(eth_account_id_, std::make_unique<EthTransaction>());
    meta.set_id(base::StrCat({chain_id, "001"}));
    meta.set_chain_id(chain_id);
    meta.set_status(mojom::TransactionStatus::Submitted);
    ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));
    meta.set_id(base::StrCat({chain_id, "002"}));
    meta.set_from(eth_account_id_other_);
    meta.tx()->set_nonce(uint256_t(4));
    meta.set_status(mojom::TransactionStatus::Confirmed);
    ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));
    meta.set_id(base::StrCat({chain_id, "003"}));
    meta.set_from(eth_account_id_other_);
    meta.tx()->set_nonce(uint256_t(4));
    meta.set_status(mojom::TransactionStatus::Submitted);
    ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));
    meta.set_id(base::StrCat({chain_id, "004"}));
    meta.set_from(eth_account_id_other_);
    meta.tx()->set_nonce(uint256_t(4));
    meta.set_status(mojom::TransactionStatus::Signed);
    ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));
    meta.set_id(base::StrCat({chain_id, "005"}));
    meta.set_from(eth_account_id_other_);
    meta.tx()->set_nonce(uint256_t(5));
    meta.set_status(mojom::TransactionStatus::Signed);
    ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));
  }

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

  for (const std::string& chain_id :
       {mojom::kMainnetChainId, mojom::kSepoliaChainId}) {
    std::set<std::string> pending_chain_ids;
    EXPECT_TRUE(pending_tx_tracker.UpdatePendingTransactions(
        chain_id, &pending_chain_ids));
    EXPECT_EQ(1UL, pending_chain_ids.size());
    WaitForResponse();
    auto meta_from_state =
        tx_state_manager_->GetEthTx(base::StrCat({chain_id, "001"}));
    ASSERT_NE(meta_from_state, nullptr);
    EXPECT_EQ(meta_from_state->status(), mojom::TransactionStatus::Confirmed);
    EXPECT_EQ(meta_from_state->from(), eth_account_id_);
    EXPECT_EQ(meta_from_state->tx_receipt().contract_address,
              "0xb60e8dd61c5d32be8058bb8eb970870f07233155");

    meta_from_state =
        tx_state_manager_->GetEthTx(base::StrCat({chain_id, "003"}));
    ASSERT_EQ(meta_from_state, nullptr);
    meta_from_state =
        tx_state_manager_->GetEthTx(base::StrCat({chain_id, "004"}));
    ASSERT_EQ(meta_from_state, nullptr);
    meta_from_state =
        tx_state_manager_->GetEthTx(base::StrCat({chain_id, "005"}));
    ASSERT_NE(meta_from_state, nullptr);
    EXPECT_EQ(meta_from_state->status(), mojom::TransactionStatus::Confirmed);
    EXPECT_EQ(meta_from_state->tx_receipt().contract_address,
              "0xb60e8dd61c5d32be8058bb8eb970870f07233155");
  }
}

}  // namespace brave_wallet
