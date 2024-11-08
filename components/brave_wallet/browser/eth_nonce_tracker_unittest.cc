/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"

#include <memory>
#include <string>

#include "base/files/scoped_temp_dir.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
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
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class EthNonceTrackerUnitTest : public testing::Test {
 public:
  EthNonceTrackerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
  }

  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());

    network_manager_ = std::make_unique<NetworkManager>(GetPrefs());
    json_rpc_service_ = std::make_unique<JsonRpcService>(
        shared_url_loader_factory(), network_manager_.get(), GetPrefs(),
        nullptr);
  }

  PrefService* GetPrefs() { return &prefs_; }

  network::SharedURLLoaderFactory* shared_url_loader_factory() {
    return url_loader_factory_.GetSafeWeakWrapper().get();
  }

  void WaitForResponse() { task_environment_.RunUntilIdle(); }

  void GetNextNonce(EthNonceTracker* tracker,
                    const std::string& chain_id,
                    const mojom::AccountIdPtr& from,
                    bool expected_success,
                    uint256_t expected_nonce) {
    base::RunLoop run_loop;
    tracker->GetNextNonce(
        chain_id, from,
        base::BindLambdaForTesting([&](bool success, uint256_t nonce) {
          EXPECT_EQ(expected_success, success);
          EXPECT_EQ(expected_nonce, nonce);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void SetTransactionCount(uint256_t count) {
    transaction_count_ = count;
    url_loader_factory_.ClearResponses();

    // See JsonRpcService::SetNetwork() to better understand where the
    // http://localhost:7545 URL used below is coming from.
    url_loader_factory_.AddResponse(
        network_manager_
            ->GetNetworkURL(mojom::kLocalhostChainId, mojom::CoinType::ETH)
            .spec(),
        GetResultString());
    url_loader_factory_.AddResponse(
        network_manager_
            ->GetNetworkURL(mojom::kMainnetChainId, mojom::CoinType::ETH)
            .spec(),
        GetResultString());
  }
  JsonRpcService* json_rpc_service() { return json_rpc_service_.get(); }

 private:
  std::string GetResultString() const {
    return "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"" +
           Uint256ValueToHex(transaction_count_) + "\"}";
  }

  uint256_t transaction_count_ = 0;
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(EthNonceTrackerUnitTest, GetNonce) {
  base::ScopedTempDir temp_dir;
  scoped_refptr<value_store::TestValueStoreFactory> factory =
      GetTestValueStoreFactory(temp_dir);
  std::unique_ptr<TxStorageDelegateImpl> delegate =
      GetTxStorageDelegateForTest(GetPrefs(), factory);
  auto account_resolver_delegate =
      std::make_unique<AccountResolverDelegateForTest>();
  EthTxStateManager tx_state_manager(*delegate, *account_resolver_delegate);
  EthNonceTracker nonce_tracker(&tx_state_manager, json_rpc_service());

  SetTransactionCount(2);

  auto eth_acc = account_resolver_delegate->RegisterAccount(
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived,
                    "0x2f015c60e0be116b1f0cd534704db9c92118fb6a"));

  // tx count: 2, confirmed: null, pending: null
  GetNextNonce(&nonce_tracker, mojom::kLocalhostChainId, eth_acc, true, 2);

  // tx count: 2, confirmed: [2], pending: null
  EthTxMeta meta(eth_acc, std::make_unique<EthTransaction>());
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_chain_id(mojom::kLocalhostChainId);
  meta.set_status(mojom::TransactionStatus::Confirmed);
  meta.tx()->set_nonce(uint256_t(2));
  ASSERT_TRUE(tx_state_manager.AddOrUpdateTx(meta));

  GetNextNonce(&nonce_tracker, mojom::kLocalhostChainId, eth_acc, true, 3);

  // tx count: 2, confirmed: [2, 3], pending: null
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_status(mojom::TransactionStatus::Confirmed);
  meta.tx()->set_nonce(uint256_t(3));
  ASSERT_TRUE(tx_state_manager.AddOrUpdateTx(meta));

  GetNextNonce(&nonce_tracker, mojom::kLocalhostChainId, eth_acc, true, 4);

  // tx count: 2, confirmed: [2, 3], pending: [4, 4]
  meta.set_status(mojom::TransactionStatus::Submitted);
  meta.tx()->set_nonce(uint256_t(4));
  meta.set_id(TxMeta::GenerateMetaID());
  ASSERT_TRUE(tx_state_manager.AddOrUpdateTx(meta));
  meta.set_id(TxMeta::GenerateMetaID());
  ASSERT_TRUE(tx_state_manager.AddOrUpdateTx(meta));

  GetNextNonce(&nonce_tracker, mojom::kLocalhostChainId, eth_acc, true, 5);

  // tx count: 2, confirmed: [2, 3], pending: [4, 4], sign: [5]
  meta.set_status(mojom::TransactionStatus::Signed);
  meta.set_id(TxMeta::GenerateMetaID());
  ASSERT_TRUE(tx_state_manager.AddOrUpdateTx(meta));

  GetNextNonce(&nonce_tracker, mojom::kLocalhostChainId, eth_acc, true, 5);

  // tx count: 2, confirmed: null, pending: null (mainnet)
  GetNextNonce(&nonce_tracker, mojom::kMainnetChainId, eth_acc, true, 2);
}

}  // namespace brave_wallet
