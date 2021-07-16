/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_nonce_tracker.h"

#include "base/bind.h"
#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_types.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
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

class EthNonceTrackerUnitTest : public testing::Test {
 public:
  EthNonceTrackerUnitTest()
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

  void WaitForResponse() { task_environment_.RunUntilIdle(); }

  void SetTransactionCount(uint256_t count) {
    transaction_count_ = count;
    url_loader_factory_.ClearResponses();
    url_loader_factory_.AddResponse(
        "https://mainnet-infura.brave.com/f7106c838853428280fa0c585acc9485",
        GetResultString());
  }

 private:
  std::string GetResultString() const {
    return "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"" +
           Uint256ValueToHex(transaction_count_) + "\"}";
  }

  uint256_t transaction_count_ = 0;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  content::BrowserTaskEnvironment task_environment_;
  TestingProfileManager testing_profile_manager_;
  base::ScopedTempDir temp_dir_;
};

TEST_F(EthNonceTrackerUnitTest, GetNonce) {
  EthJsonRpcController controller(Network::kMainnet,
                                  shared_url_loader_factory());
  EthTxStateManager tx_state_manager(GetPrefs());
  EthNonceTracker nonce_tracker(&tx_state_manager, &controller);

  SetTransactionCount(2);

  const std::string addr("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  uint256_t nonce_result = 0;
  bool callback_called = false;
  // tx count: 2, confirmed: null, pending: null
  nonce_tracker.GetNextNonce(
      EthAddress::FromHex(addr),
      base::BindLambdaForTesting([&](bool success, uint256_t nonce) {
        callback_called = true;
        if (success)
          nonce_result = nonce;
      }));
  WaitForResponse();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(nonce_result, uint256_t(2));

  // tx count: 2, confirmed: [2], pending: null
  EthTxStateManager::TxMeta meta;
  meta.id = EthTxStateManager::GenerateMetaID();
  meta.from = EthAddress::FromHex(addr);
  meta.status = EthTxStateManager::TransactionStatus::CONFIRMED;
  meta.tx.set_nonce(uint256_t(2));
  tx_state_manager.AddOrUpdateTx(meta);

  nonce_result = 0;
  callback_called = false;
  nonce_tracker.GetNextNonce(
      EthAddress::FromHex(addr),
      base::BindLambdaForTesting([&](bool success, uint256_t nonce) {
        callback_called = true;
        if (success)
          nonce_result = nonce;
      }));
  WaitForResponse();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(nonce_result, uint256_t(3));

  // tx count: 2, confirmed: [2, 3], pending: null
  meta.id = EthTxStateManager::GenerateMetaID();
  meta.status = EthTxStateManager::TransactionStatus::CONFIRMED;
  meta.tx.set_nonce(uint256_t(3));
  tx_state_manager.AddOrUpdateTx(meta);

  nonce_result = 0;
  callback_called = false;
  nonce_tracker.GetNextNonce(
      EthAddress::FromHex(addr),
      base::BindLambdaForTesting([&](bool success, uint256_t nonce) {
        callback_called = true;
        if (success)
          nonce_result = nonce;
      }));
  WaitForResponse();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(nonce_result, uint256_t(4));

  // tx count: 2, confirmed: [2, 3], pending: [4, 4]
  meta.status = EthTxStateManager::TransactionStatus::SUBMITTED;
  meta.tx.set_nonce(uint256_t(4));
  meta.id = EthTxStateManager::GenerateMetaID();
  tx_state_manager.AddOrUpdateTx(meta);
  meta.id = EthTxStateManager::GenerateMetaID();
  tx_state_manager.AddOrUpdateTx(meta);

  nonce_result = 0;
  callback_called = false;
  nonce_tracker.GetNextNonce(
      EthAddress::FromHex(addr),
      base::BindLambdaForTesting([&](bool success, uint256_t nonce) {
        callback_called = true;
        if (success)
          nonce_result = nonce;
      }));
  WaitForResponse();
  EXPECT_TRUE(callback_called);
  EXPECT_EQ(nonce_result, uint256_t(5));
}

TEST_F(EthNonceTrackerUnitTest, NonceLock) {
  EthJsonRpcController controller(Network::kMainnet,
                                  shared_url_loader_factory());
  EthTxStateManager tx_state_manager(GetPrefs());
  EthNonceTracker nonce_tracker(&tx_state_manager, &controller);

  SetTransactionCount(4);

  base::Lock* lock = nonce_tracker.GetLock();
  lock->Acquire();
  const std::string addr("0x2f015c60e0be116b1f0cd534704db9c92118fb6a");
  bool callback_called = false;
  bool callback_success;
  nonce_tracker.GetNextNonce(
      EthAddress::FromHex(addr),
      base::BindLambdaForTesting([&](bool success, uint256_t nonce) {
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
      EthAddress::FromHex(addr),
      base::BindLambdaForTesting([&](bool success, uint256_t nonce) {
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
