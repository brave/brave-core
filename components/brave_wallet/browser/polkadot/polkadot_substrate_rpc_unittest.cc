/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"

#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
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
    RegisterLocalStatePrefs(local_state_.registry());

    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    polkadot_substrate_rpc_ = std::make_unique<PolkadotSubstrateRpc>(
        *network_manager_, shared_url_loader_factory_);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<PolkadotSubstrateRpc> polkadot_substrate_rpc_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
};

TEST_F(PolkadotSubstrateRpcUnitTest, GetChainName) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitWithFeaturesAndParameters(
      {{features::kBraveWalletPolkadotFeature, {}}}, {});

  url_loader_factory_.ClearResponses();

  const auto* chain_id = mojom::kPolkadotTestnet;
  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");

  base::test::TestFuture<const std::optional<std::string>&,
                         const std::optional<std::string>&>
      future;

  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "result": "Westend",
      "id": 1 })");
  polkadot_substrate_rpc_->GetChainName(chain_id, future.GetCallback());

  auto [network_name, error] = future.Take();
  EXPECT_EQ(network_name, "Westend");
  EXPECT_EQ(error, std::nullopt);

  url_loader_factory_.AddResponse(testnet_url, R"(
    { "jsonrpc": "2.0",
      "not_result": "westend",
      "id": 1 })");
  polkadot_substrate_rpc_->GetChainName(chain_id, future.GetCallback());

  std::tie(network_name, error) = future.Take();
  EXPECT_EQ(network_name, std::nullopt);
  EXPECT_EQ(error, WalletParsingErrorMessage());

  url_loader_factory_.AddResponse(testnet_url, R"(
    { "id": 1 })");
  polkadot_substrate_rpc_->GetChainName(chain_id, future.GetCallback());

  std::tie(network_name, error) = future.Take();
  EXPECT_EQ(network_name, std::nullopt);
  EXPECT_EQ(error, WalletParsingErrorMessage());

  url_loader_factory_.AddResponse(testnet_url, R"(
    {"jsonrpc":"2.0",
     "id":1,
     "error":{"code":-32601,"message":"Method not found"}}
  )");
  polkadot_substrate_rpc_->GetChainName(chain_id, future.GetCallback());

  std::tie(network_name, error) = future.Take();
  EXPECT_EQ(network_name, std::nullopt);
  EXPECT_EQ(error, "Method not found");

  url_loader_factory_.AddResponse(testnet_url, R"(
    {"jsonrpc":"2.0",
     "id":1,
     "error":{"code":-32601}}
  )");
  polkadot_substrate_rpc_->GetChainName(chain_id, future.GetCallback());

  std::tie(network_name, error) = future.Take();
  EXPECT_EQ(network_name, std::nullopt);
  EXPECT_EQ(error, "An internal error has occurred");

  url_loader_factory_.AddResponse(testnet_url, "",
                                  net::HTTP_INTERNAL_SERVER_ERROR);
  polkadot_substrate_rpc_->GetChainName(chain_id, future.GetCallback());

  std::tie(network_name, error) = future.Take();
  EXPECT_EQ(network_name, std::nullopt);
  EXPECT_EQ(error, WalletInternalErrorMessage());
}

}  // namespace brave_wallet
