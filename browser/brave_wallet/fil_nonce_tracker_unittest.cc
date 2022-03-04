/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_nonce_tracker.h"

#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/fil_tx_meta.h"
#include "brave/components/brave_wallet/browser/fil_tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class FilNonceTrackerUnitTest : public testing::Test {
 public:
  FilNonceTrackerUnitTest() {
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

  void WaitForResponse() { task_environment_.RunUntilIdle(); }

  void SetTransactionCount(uint256_t count) {
    transaction_count_ = count;
    url_loader_factory_.ClearResponses();

    // See JsonRpcService::SetNetwork() to better understand where the
    // http://localhost:7545 URL used below is coming from.
    url_loader_factory_.AddResponse(
        brave_wallet::GetNetworkURL(GetPrefs(), mojom::kLocalhostChainId,
                                    mojom::CoinType::FIL)
            .spec(),
        GetResultString());
  }

 private:
  std::string GetResultString() const {
    return "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":" +
           base::NumberToString(transaction_count_) + "}";
  }

  uint64_t transaction_count_ = 0;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(FilNonceTrackerUnitTest, GetNonce) {
  JsonRpcService service(shared_url_loader_factory(), GetPrefs());
  base::RunLoop run_loop;
  service.SetNetwork(
      brave_wallet::mojom::kLocalhostChainId, mojom::CoinType::FIL,
      base::BindLambdaForTesting([&](bool success) { run_loop.Quit(); }));
  run_loop.Run();

  FilTxStateManager tx_state_manager(GetPrefs(), &service);
  FilNonceTracker nonce_tracker(&tx_state_manager, &service);

  SetTransactionCount(2);

  const std::string addr("t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy");
  uint256_t nonce_result = 0;
  bool callback_called = false;
  // tx count: 2, confirmed: null, pending: null
  nonce_tracker.GetNextNonce(
      addr, base::BindLambdaForTesting([&](bool success, uint256_t nonce) {
        callback_called = true;
        if (success)
          nonce_result = nonce;
      }));
  WaitForResponse();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(nonce_result, uint64_t(2));

  // tx count: 2, confirmed: [2], pending: null
  FilTxMeta meta;
  meta.set_id(TxMeta::GenerateMetaID());

  meta.set_from(FilAddress::From(addr).ToChecksumAddress());
  meta.set_status(mojom::TransactionStatus::Confirmed);
  meta.tx()->set_nonce(uint64_t(2));
  tx_state_manager.AddOrUpdateTx(meta);

  nonce_result = 0;
  callback_called = false;
  nonce_tracker.GetNextNonce(
      addr, base::BindLambdaForTesting([&](bool success, uint256_t nonce) {
        callback_called = true;
        if (success)
          nonce_result = nonce;
      }));
  WaitForResponse();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(nonce_result, uint256_t(3));

  // tx count: 2, confirmed: [2, 3], pending: null
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_status(mojom::TransactionStatus::Confirmed);
  meta.tx()->set_nonce(uint256_t(3));
  tx_state_manager.AddOrUpdateTx(meta);

  nonce_result = 0;
  callback_called = false;
  nonce_tracker.GetNextNonce(
      addr, base::BindLambdaForTesting([&](bool success, uint256_t nonce) {
        callback_called = true;
        if (success)
          nonce_result = nonce;
      }));
  WaitForResponse();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(nonce_result, uint256_t(4));

  // tx count: 2, confirmed: [2, 3], pending: [4, 4]
  meta.set_status(mojom::TransactionStatus::Submitted);
  meta.tx()->set_nonce(uint256_t(4));
  meta.set_id(TxMeta::GenerateMetaID());
  tx_state_manager.AddOrUpdateTx(meta);
  meta.set_id(TxMeta::GenerateMetaID());
  tx_state_manager.AddOrUpdateTx(meta);

  nonce_result = 0;
  callback_called = false;
  nonce_tracker.GetNextNonce(
      addr, base::BindLambdaForTesting([&](bool success, uint256_t nonce) {
        callback_called = true;
        if (success)
          nonce_result = nonce;
      }));
  WaitForResponse();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(nonce_result, uint256_t(5));
}

TEST_F(FilNonceTrackerUnitTest, NonceLock) {
  JsonRpcService service(shared_url_loader_factory(), GetPrefs());
  base::RunLoop run_loop;
  service.SetNetwork(
      brave_wallet::mojom::kLocalhostChainId, mojom::CoinType::FIL,
      base::BindLambdaForTesting([&](bool success) { run_loop.Quit(); }));
  run_loop.Run();
  FilTxStateManager tx_state_manager(GetPrefs(), &service);
  FilNonceTracker nonce_tracker(&tx_state_manager, &service);

  SetTransactionCount(4);

  base::Lock* lock = nonce_tracker.GetLock();
  lock->Acquire();
  const std::string addr("t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy");
  bool callback_called = false;
  bool callback_success;
  nonce_tracker.GetNextNonce(
      addr, base::BindLambdaForTesting([&](bool success, uint256_t nonce) {
        callback_called = true;
        callback_success = success;
      }));
  WaitForResponse();
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(callback_success);

  lock->Release();

  uint256_t nonce_result = 0;
  callback_called = false;
  nonce_tracker.GetNextNonce(
      addr, base::BindLambdaForTesting([&](bool success, uint256_t nonce) {
        callback_called = true;
        callback_success = success;
        nonce_result = nonce;
      }));
  WaitForResponse();
  EXPECT_TRUE(callback_called);
  EXPECT_TRUE(callback_success);
  EXPECT_EQ(nonce_result, uint256_t(4));
}

}  // namespace brave_wallet
