/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"

#include <memory>
#include <string>
#include <utility>

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/eth_tx_meta.h"
#include "brave/components/brave_wallet/browser/eth_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
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
    brave_wallet::RegisterProfilePrefs(prefs_.registry());
  }

  PrefService* GetPrefs() { return &prefs_; }

  network::SharedURLLoaderFactory* shared_url_loader_factory() {
    return url_loader_factory_.GetSafeWeakWrapper().get();
  }

  void WaitForResponse() { task_environment_.RunUntilIdle(); }

  void GetNextNonce(EthNonceTracker* tracker,
                    const std::string& address,
                    bool expected_success,
                    uint256_t expected_nonce) {
    base::RunLoop run_loop;
    tracker->GetNextNonce(
        address, base::BindLambdaForTesting([&](bool success, uint256_t nonce) {
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
        brave_wallet::GetNetworkURL(GetPrefs(), mojom::kLocalhostChainId,
                                    mojom::CoinType::ETH)
            .spec(),
        GetResultString());
  }

 private:
  std::string GetResultString() const {
    return "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"" +
           Uint256ValueToHex(transaction_count_) + "\"}";
  }

  uint256_t transaction_count_ = 0;
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(EthNonceTrackerUnitTest, GetNonce) {
  JsonRpcService service(shared_url_loader_factory(), GetPrefs());
  base::RunLoop run_loop;
  service.SetNetwork(
      brave_wallet::mojom::kLocalhostChainId, mojom::CoinType::ETH,
      base::BindLambdaForTesting([&](bool success) { run_loop.Quit(); }));
  run_loop.Run();

  EthTxStateManager tx_state_manager(GetPrefs(), &service);
  EthNonceTracker nonce_tracker(&tx_state_manager, &service);

  SetTransactionCount(2);

  const std::string address("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  // tx count: 2, confirmed: null, pending: null
  GetNextNonce(&nonce_tracker, address, true, 2);

  // tx count: 2, confirmed: [2], pending: null
  EthTxMeta meta;
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_from(EthAddress::FromHex(address).ToChecksumAddress());
  meta.set_status(mojom::TransactionStatus::Confirmed);
  meta.tx()->set_nonce(uint256_t(2));
  tx_state_manager.AddOrUpdateTx(meta);

  GetNextNonce(&nonce_tracker, address, true, 3);

  // tx count: 2, confirmed: [2, 3], pending: null
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_status(mojom::TransactionStatus::Confirmed);
  meta.tx()->set_nonce(uint256_t(3));
  tx_state_manager.AddOrUpdateTx(meta);

  GetNextNonce(&nonce_tracker, address, true, 4);

  // tx count: 2, confirmed: [2, 3], pending: [4, 4]
  meta.set_status(mojom::TransactionStatus::Submitted);
  meta.tx()->set_nonce(uint256_t(4));
  meta.set_id(TxMeta::GenerateMetaID());
  tx_state_manager.AddOrUpdateTx(meta);
  meta.set_id(TxMeta::GenerateMetaID());
  tx_state_manager.AddOrUpdateTx(meta);

  GetNextNonce(&nonce_tracker, address, true, 5);

  // tx count: 2, confirmed: [2, 3], pending: [4, 4], sign: [5]
  meta.set_status(mojom::TransactionStatus::Signed);
  meta.set_id(TxMeta::GenerateMetaID());
  tx_state_manager.AddOrUpdateTx(meta);

  GetNextNonce(&nonce_tracker, address, true, 5);
}

TEST_F(EthNonceTrackerUnitTest, NonceLock) {
  JsonRpcService service(shared_url_loader_factory(), GetPrefs());
  base::RunLoop run_loop;
  service.SetNetwork(
      brave_wallet::mojom::kLocalhostChainId, mojom::CoinType::ETH,
      base::BindLambdaForTesting([&](bool success) { run_loop.Quit(); }));
  run_loop.Run();
  EthTxStateManager tx_state_manager(GetPrefs(), &service);
  EthNonceTracker nonce_tracker(&tx_state_manager, &service);

  SetTransactionCount(4);

  base::Lock* lock = nonce_tracker.GetLock();
  lock->Acquire();
  const std::string addr("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  GetNextNonce(&nonce_tracker, addr, false, 0);

  lock->Release();

  GetNextNonce(&nonce_tracker, addr, true, 4);
}

}  // namespace brave_wallet
