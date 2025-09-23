/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"

#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class PolkadotSubstrateRpcUnitTest : public testing::Test {
 public:
  PolkadotSubstrateRpcUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)) {}

  ~PolkadotSubstrateRpcUnitTest() override = default;

  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    polkadot_substrate_rpc_ = std::make_unique<PolkadotSubstrateRpc>(
        *network_manager_, shared_url_loader_factory_);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<PolkadotSubstrateRpc> polkadot_substrate_rpc_;
};

TEST_F(PolkadotSubstrateRpcUnitTest, GetChainName) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{features::kBraveWalletPolkadotFeature, {}}}, {});

  base::MockCallback<PolkadotSubstrateRpc::GetChainNameCallback> callback;

  url_loader_factory_.ClearResponses();

  const auto* chain_id = mojom::kPolkadotTestnet;
  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_CALL(callback, Run(std::optional<std::string>("westend"),
                            std::optional<std::string>(std::nullopt)));
  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "westend",
      "id": 1 })");
  polkadot_substrate_rpc_->GetChainName(chain_id, callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_CALL(callback,
              Run(std::optional<std::string>(std::nullopt),
                  std::optional<std::string>(WalletParsingErrorMessage())));
  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "not_result": "westend",
      "id": 1 })");
  polkadot_substrate_rpc_->GetChainName(chain_id, callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_CALL(callback,
              Run(std::optional<std::string>(std::nullopt),
                  std::optional<std::string>(WalletParsingErrorMessage())));
  url_loader_factory_.AddResponse(testnet_url, R"(
    { "id": 1 })");
  polkadot_substrate_rpc_->GetChainName(chain_id, callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_CALL(callback, Run(std::optional<std::string>(std::nullopt),
                            std::optional<std::string>("Method not found")));
  url_loader_factory_.AddResponse(testnet_url, R"(
    {"jsonrpc":"2.0","id":1,"error":{"code":-32601,"message":"Method not found"}}
  )");
  polkadot_substrate_rpc_->GetChainName(chain_id, callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);

  EXPECT_CALL(callback,
              Run(std::optional<std::string>(std::nullopt),
                  std::optional<std::string>(WalletInternalErrorMessage())));
  url_loader_factory_.AddResponse(testnet_url, "",
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  polkadot_substrate_rpc_->GetChainName(chain_id, callback.Get());
  task_environment_.RunUntilIdle();
  testing::Mock::VerifyAndClearExpectations(&callback);
}

}  // namespace brave_wallet
