/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_pending_tx_tracker.h"

#include <memory>
#include <set>
#include <string>
#include <string_view>

#include "base/files/scoped_temp_dir.h"
#include "base/strings/strcat.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_meta.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/browser/tx_storage.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

constexpr char kTxHash[] =
    "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238";

constexpr char kFailedReceiptResponse[] = R"({
  "jsonrpc": "2.0",
  "id": 1,
  "result": {
    "transactionHash": "0xb903",
    "transactionIndex": "0x1",
    "blockNumber": "0xb",
    "blockHash": "0xc6ef",
    "cumulativeGasUsed": "0x33bc",
    "gasUsed": "0x4dc",
    "contractAddress": "0xb60e8dd61c5d32be8058bb8eb970870f07233155",
    "logs": [],
    "logsBloom": "0x00...0",
    "status": "0x0"
  }
})";

}  // namespace

class EthPendingTxTrackerUnitTest : public testing::Test {
 public:
  EthPendingTxTrackerUnitTest() = default;

  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());

    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    tx_storage_ = CreateTxStorageForTest(temp_dir_.GetPath());
    account_resolver_delegate_ =
        std::make_unique<AccountResolverDelegateForTest>();
    tx_state_manager_ = std::make_unique<EthTxStateManager>(
        *tx_storage_, *account_resolver_delegate_);

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
        url_loader_factory_.GetSafeWeakWrapper(), network_manager_.get(),
        GetPrefs(), nullptr);
  }

  JsonRpcService* json_rpc_service() { return json_rpc_service_.get(); }

  PrefService* GetPrefs() { return &prefs_; }

  network::TestURLLoaderFactory* test_url_loader_factory() {
    return &url_loader_factory_;
  }

  // Answers eth_getTransactionReceipt with a failed receipt and
  // eth_getTransactionCount with `network_nonce_hex`, counting both requests.
  void InterceptRpcRequests(const std::string& network_nonce_hex) {
    test_url_loader_factory()->SetInterceptor(base::BindLambdaForTesting(
        [&, network_nonce_hex](const network::ResourceRequest& request) {
          base::DictValue body =
              base::test::ParseJsonDict(network::GetUploadData(request));
          const std::string* method = body.FindString("method");
          ASSERT_TRUE(method);
          if (*method == "eth_getTransactionReceipt") {
            ++receipt_requests_;
            test_url_loader_factory()->AddResponse(request.url.spec(),
                                                   kFailedReceiptResponse);
          } else if (*method == "eth_getTransactionCount") {
            ++nonce_requests_;
            test_url_loader_factory()->AddResponse(
                request.url.spec(),
                base::StrCat({R"({"jsonrpc":"2.0","id":1,"result":")",
                              network_nonce_hex, R"("})"}));
          } else {
            FAIL() << "Unexpected method: " << *method;
          }
        }));
  }

  void AddSubmittedTx(const std::string& id, uint64_t nonce) {
    EthTxMeta meta(eth_account_id_, std::make_unique<EthTransaction>());
    meta.set_id(id);
    meta.set_chain_id(mojom::kMainnetChainId);
    meta.set_tx_hash(kTxHash);
    meta.tx()->set_nonce(uint256_t(nonce));
    meta.set_status(mojom::TransactionStatus::Submitted);
    ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));
  }

  void UpdateMainnetAndWaitForReplies(EthPendingTxTracker& tracker) {
    std::set<std::string> pending_chain_ids;
    ASSERT_TRUE(tracker.UpdatePendingTransactions(mojom::kMainnetChainId,
                                                  &pending_chain_ids));
    // Receipt handlers may start another RPC for the network nonce. Both
    // callbacks run on this sequence before it can observe zero requests.
    ASSERT_TRUE(base::test::RunUntil([&] {
      return !json_rpc_service()->HasPendingRpcRequestsForTesting();
    }));
  }

 private:
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<TxStorage> tx_storage_;
  std::unique_ptr<AccountResolverDelegateForTest> account_resolver_delegate_;

 protected:
  std::unique_ptr<EthTxStateManager> tx_state_manager_;
  mojom::AccountIdPtr eth_account_id_;
  mojom::AccountIdPtr eth_account_id_other_;
  int receipt_requests_ = 0;
  int nonce_requests_ = 0;
};

TEST_F(EthPendingTxTrackerUnitTest, DropsPendingTxWhenNonceTaken) {
  EthNonceTracker nonce_tracker(tx_state_manager_.get(), json_rpc_service());
  EthPendingTxTracker pending_tx_tracker(tx_state_manager_.get(),
                                         json_rpc_service(), &nonce_tracker);

  EthTxMeta meta(eth_account_id_, std::make_unique<EthTransaction>());
  meta.set_chain_id(mojom::kMainnetChainId);
  meta.tx()->set_nonce(uint256_t(123));
  meta.set_id("mainnet_confirmed");
  meta.set_status(mojom::TransactionStatus::Confirmed);
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));
  meta.set_id("mainnet_pending");
  meta.set_status(mojom::TransactionStatus::Submitted);
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));
  meta.set_id("sepolia_pending");
  meta.set_chain_id(mojom::kSepoliaChainId);
  ASSERT_TRUE(tx_state_manager_->AddOrUpdateTx(meta));

  // The pending tx shares its nonce with a confirmed tx on the same chain, so
  // it is dropped without querying the network.
  std::set<std::string> pending_chain_ids;
  EXPECT_TRUE(pending_tx_tracker.UpdatePendingTransactions(
      mojom::kMainnetChainId, &pending_chain_ids));
  EXPECT_TRUE(pending_chain_ids.empty());
  EXPECT_EQ(tx_state_manager_->GetTx("mainnet_pending"), nullptr);
  EXPECT_NE(tx_state_manager_->GetTx("mainnet_confirmed"), nullptr);

  // The confirmed tx on another chain does not take the nonce.
  EXPECT_TRUE(pending_tx_tracker.UpdatePendingTransactions(
      mojom::kSepoliaChainId, &pending_chain_ids));
  EXPECT_EQ(pending_chain_ids, std::set<std::string>{mojom::kSepoliaChainId});
  ASSERT_NE(tx_state_manager_->GetTx("sepolia_pending"), nullptr);
  EXPECT_EQ(tx_state_manager_->GetTx("sepolia_pending")->status(),
            mojom::TransactionStatus::Submitted);
}

TEST_F(EthPendingTxTrackerUnitTest, DropsFailedTxBelowNetworkNonce) {
  InterceptRpcRequests("0x3");
  EthNonceTracker nonce_tracker(tx_state_manager_.get(), json_rpc_service());
  EthPendingTxTracker pending_tx_tracker(tx_state_manager_.get(),
                                         json_rpc_service(), &nonce_tracker);
  AddSubmittedTx("pending", 1);

  ASSERT_NO_FATAL_FAILURE(UpdateMainnetAndWaitForReplies(pending_tx_tracker));
  ASSERT_EQ(receipt_requests_, 1);
  ASSERT_EQ(nonce_requests_, 1);
  ASSERT_NE(tx_state_manager_->GetTx("pending"), nullptr);

  // The second failed receipt sees that the cached network nonce is higher.
  ASSERT_NO_FATAL_FAILURE(UpdateMainnetAndWaitForReplies(pending_tx_tracker));
  EXPECT_EQ(tx_state_manager_->GetTx("pending"), nullptr);
  EXPECT_EQ(receipt_requests_, 2);
  EXPECT_EQ(nonce_requests_, 1);
}

TEST_F(EthPendingTxTrackerUnitTest, DropsFailedTxAfterFourthFailedReceipt) {
  InterceptRpcRequests("0x3");
  EthNonceTracker nonce_tracker(tx_state_manager_.get(), json_rpc_service());
  EthPendingTxTracker pending_tx_tracker(tx_state_manager_.get(),
                                         json_rpc_service(), &nonce_tracker);
  // The tx nonce (4) is never below the network nonce (3).
  AddSubmittedTx("pending", 4);

  for (int poll = 1; poll <= 3; ++poll) {
    SCOPED_TRACE(poll);
    ASSERT_NO_FATAL_FAILURE(UpdateMainnetAndWaitForReplies(pending_tx_tracker));
    ASSERT_EQ(receipt_requests_, poll);
    ASSERT_NE(tx_state_manager_->GetTx("pending"), nullptr);
  }

  ASSERT_NO_FATAL_FAILURE(UpdateMainnetAndWaitForReplies(pending_tx_tracker));
  EXPECT_EQ(receipt_requests_, 4);
  EXPECT_EQ(tx_state_manager_->GetTx("pending"), nullptr);
}

TEST_F(EthPendingTxTrackerUnitTest, UpdatePendingTransactions) {
  EthNonceTracker nonce_tracker(tx_state_manager_.get(), json_rpc_service());
  EthPendingTxTracker pending_tx_tracker(tx_state_manager_.get(),
                                         json_rpc_service(), &nonce_tracker);

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
    ASSERT_TRUE(base::test::RunUntil([&] {
      auto tx_001 =
          tx_state_manager_->GetEthTx(base::StrCat({chain_id, "001"}));
      auto tx_005 =
          tx_state_manager_->GetEthTx(base::StrCat({chain_id, "005"}));
      return tx_001 && tx_005 &&
             tx_001->status() == mojom::TransactionStatus::Confirmed &&
             tx_005->status() == mojom::TransactionStatus::Confirmed;
    }));
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
