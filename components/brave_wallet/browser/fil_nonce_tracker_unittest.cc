/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_nonce_tracker.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/fil_tx_meta.h"
#include "brave/components/brave_wallet/browser/fil_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class FilNonceTrackerUnitTest : public testing::Test {
 public:
  FilNonceTrackerUnitTest()
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

  void SetTransactionCount(uint64_t count) {
    transaction_count_ = count;
    url_loader_factory_.ClearResponses();

    url_loader_factory_.AddResponse(
        brave_wallet::GetNetworkURL(GetPrefs(), mojom::kLocalhostChainId,
                                    mojom::CoinType::FIL)
            .spec(),
        GetResultString());
  }

  void GetNextNonce(FilNonceTracker* tracker,
                    const std::string& address,
                    bool expected_success,
                    uint64_t expected_nonce) {
    base::RunLoop run_loop;
    tracker->GetNextNonce(
        address, base::BindLambdaForTesting([&](bool success, uint256_t nonce) {
          EXPECT_EQ(expected_success, success);
          EXPECT_EQ(expected_nonce, nonce);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

 private:
  std::string GetResultString() const {
    return "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":" +
           base::NumberToString(transaction_count_) + "}";
  }

  uint64_t transaction_count_ = 0;
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(FilNonceTrackerUnitTest, GetNonce) {
  JsonRpcService service(shared_url_loader_factory(), GetPrefs());
  base::RunLoop run_loop;
  service.SetNetwork(
      mojom::kLocalhostChainId, mojom::CoinType::FIL,
      base::BindLambdaForTesting([&](bool success) { run_loop.Quit(); }));
  run_loop.Run();

  FilTxStateManager tx_state_manager(GetPrefs(), &service);
  FilNonceTracker nonce_tracker(&tx_state_manager, &service);

  SetTransactionCount(2);

  const std::string address("t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy");
  GetNextNonce(&nonce_tracker, address, true, uint64_t(2));

  // tx count: 2, confirmed: [2], pending: null
  FilTxMeta meta;
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_from(FilAddress::FromAddress(address).EncodeAsString());
  meta.set_status(mojom::TransactionStatus::Confirmed);
  meta.tx()->set_nonce(uint64_t(2));
  tx_state_manager.AddOrUpdateTx(meta);

  GetNextNonce(&nonce_tracker, address, true, uint64_t(3));

  // tx count: 2, confirmed: [2, 3], pending: null
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_status(mojom::TransactionStatus::Confirmed);
  meta.tx()->set_nonce(uint64_t(3));
  tx_state_manager.AddOrUpdateTx(meta);

  GetNextNonce(&nonce_tracker, address, true, uint64_t(4));

  // tx count: 2, confirmed: [2, 3], pending: [4, 4]
  meta.set_status(mojom::TransactionStatus::Submitted);
  meta.tx()->set_nonce(uint64_t(4));
  meta.set_id(TxMeta::GenerateMetaID());
  tx_state_manager.AddOrUpdateTx(meta);
  meta.set_id(TxMeta::GenerateMetaID());
  tx_state_manager.AddOrUpdateTx(meta);

  GetNextNonce(&nonce_tracker, address, true, uint64_t(5));
}

TEST_F(FilNonceTrackerUnitTest, NonceLock) {
  JsonRpcService service(shared_url_loader_factory(), GetPrefs());
  base::RunLoop run_loop;
  service.SetNetwork(
      mojom::kLocalhostChainId, mojom::CoinType::FIL,
      base::BindLambdaForTesting([&](bool success) { run_loop.Quit(); }));
  run_loop.Run();
  FilTxStateManager tx_state_manager(GetPrefs(), &service);
  FilNonceTracker nonce_tracker(&tx_state_manager, &service);

  SetTransactionCount(4);

  base::Lock* lock = nonce_tracker.GetLock();
  lock->Acquire();
  const std::string address("t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy");
  GetNextNonce(&nonce_tracker, address, false, 0u);
  lock->Release();

  GetNextNonce(&nonce_tracker, address, true, uint64_t(4));
}

}  // namespace brave_wallet
