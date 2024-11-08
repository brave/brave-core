/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_nonce_tracker.h"

#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/fil_tx_meta.h"
#include "brave/components/brave_wallet/browser/fil_tx_state_manager.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/browser/tx_storage_delegate_impl.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
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

  void SetTransactionCount(uint64_t count) {
    transaction_count_ = count;
    url_loader_factory_.ClearResponses();

    url_loader_factory_.AddResponse(
        network_manager_
            ->GetNetworkURL(mojom::kLocalhostChainId, mojom::CoinType::FIL)
            .spec(),
        GetResultString());
    url_loader_factory_.AddResponse(
        network_manager_
            ->GetNetworkURL(mojom::kFilecoinMainnet, mojom::CoinType::FIL)
            .spec(),
        GetResultString());
  }

  void GetNextNonce(const base::Location& location,
                    FilNonceTracker* tracker,
                    const std::string& chain_id,
                    const mojom::AccountIdPtr& from,
                    bool expected_success,
                    uint64_t expected_nonce) {
    SCOPED_TRACE(testing::Message() << location.ToString());
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

  JsonRpcService* json_rpc_service() { return json_rpc_service_.get(); }

 private:
  std::string GetResultString() const {
    return "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":" +
           base::NumberToString(transaction_count_) + "}";
  }

  uint64_t transaction_count_ = 0;
  base::test::TaskEnvironment task_environment_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(FilNonceTrackerUnitTest, GetNonce) {
  base::ScopedTempDir temp_dir;
  scoped_refptr<value_store::TestValueStoreFactory> factory =
      GetTestValueStoreFactory(temp_dir);
  std::unique_ptr<TxStorageDelegateImpl> delegate =
      GetTxStorageDelegateForTest(GetPrefs(), factory);
  auto account_resolver_delegate =
      std::make_unique<AccountResolverDelegateForTest>();
  FilTxStateManager tx_state_manager(*delegate, *account_resolver_delegate);
  FilNonceTracker nonce_tracker(&tx_state_manager, json_rpc_service());

  SetTransactionCount(2);

  const std::string address("t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy");
  auto fil_acc = account_resolver_delegate->RegisterAccount(
      MakeAccountId(mojom::CoinType::FIL, mojom::KeyringId::kFilecoinTestnet,
                    mojom::AccountKind::kDerived,
                    "t1lqarsh4nkg545ilaoqdsbtj4uofplt6sto26ziy"));
  GetNextNonce(FROM_HERE, &nonce_tracker, mojom::kLocalhostChainId, fil_acc,
               true, uint64_t(2));

  // tx count: 2, confirmed: [2], pending: null
  FilTxMeta meta(fil_acc, std::make_unique<FilTransaction>());
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_chain_id(mojom::kLocalhostChainId);
  meta.set_status(mojom::TransactionStatus::Confirmed);
  meta.tx()->set_nonce(uint64_t(2));
  ASSERT_TRUE(tx_state_manager.AddOrUpdateTx(meta));

  GetNextNonce(FROM_HERE, &nonce_tracker, mojom::kLocalhostChainId, fil_acc,
               true, uint64_t(3));

  // tx count: 2, confirmed: [2, 3], pending: null
  meta.set_id(TxMeta::GenerateMetaID());
  meta.set_status(mojom::TransactionStatus::Confirmed);
  meta.tx()->set_nonce(uint64_t(3));
  ASSERT_TRUE(tx_state_manager.AddOrUpdateTx(meta));

  GetNextNonce(FROM_HERE, &nonce_tracker, mojom::kLocalhostChainId, fil_acc,
               true, uint64_t(4));

  // tx count: 2, confirmed: [2, 3], pending: [4, 4]
  meta.set_status(mojom::TransactionStatus::Submitted);
  meta.tx()->set_nonce(uint64_t(4));
  meta.set_id(TxMeta::GenerateMetaID());
  ASSERT_TRUE(tx_state_manager.AddOrUpdateTx(meta));
  meta.set_id(TxMeta::GenerateMetaID());
  ASSERT_TRUE(tx_state_manager.AddOrUpdateTx(meta));

  GetNextNonce(FROM_HERE, &nonce_tracker, mojom::kLocalhostChainId, fil_acc,
               true, uint64_t(5));

  // tx count: 2, confirmed: null, pending: null (mainnet)
  GetNextNonce(FROM_HERE, &nonce_tracker, mojom::kFilecoinMainnet, fil_acc,
               true, uint64_t(2));
}

}  // namespace brave_wallet
