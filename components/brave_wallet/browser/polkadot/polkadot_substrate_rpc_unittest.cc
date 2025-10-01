/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"

#include "base/json/json_reader.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc_responses.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
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

TEST_F(PolkadotSubstrateRpcUnitTest, GetAccountBalance) {
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

  base::test::TestFuture<mojom::PolkadotAccountInfoPtr,
                         const std::optional<std::string>&>
      future;

  constexpr const char kPubKey[] =
      "D43593C715FDD31C61141ABD04A99FD6822C8558854CCDE39A5684E7A56DA27D";

  std::vector<uint8_t> out;
  base::HexStringToBytes(kPubKey, &out);

  base::span<const uint8_t, kSr25519PublicKeySize> pubkey(out);

  {
    // Account exists.

    polkadot_substrate_rpc_->GetAccountBalance(chain_id, pubkey,
                                               future.GetCallback());

    auto* reqs = url_loader_factory_.pending_requests();
    EXPECT_TRUE(reqs);
    EXPECT_EQ(reqs->size(), 1u);

    auto const& req = reqs->at(0);
    EXPECT_TRUE(req.request.request_body->elements());
    auto const& element = req.request.request_body->elements()->at(0);
    EXPECT_EQ(
        element.As<network::DataElementBytes>().AsStringPiece(),
        "{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"state_queryStorageAt\","
        "\"params\":[["
        "\"0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371D"
        "A9DE1E86A9A8C739864CF3CC5EC2BEA59FD43593C715FDD31C61141ABD04A99FD6"
        "822C8558854CCDE39A5684E7A56DA27D\"]]}");

    url_loader_factory_.AddResponse(
        testnet_url,
        R"({"id":1,"jsonrpc":"2.0","result":[{"block":"0x1bcd3e074b91ef25740714dc63671f4a36d2781ff93877ef9ef31b849d1ad69c","changes":[["0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9de1e86a9a8c739864cf3cc5ec2bea59fd43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d","0x76030000020000000100000000000000b18ac01b0300000000000000000000008030a55c79b5000000000000000000000000000000000000000000000000000000000000000000000000000000000080"]]}]})");

    auto [account_info, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    EXPECT_EQ(account_info->nonce, 886u);
    EXPECT_EQ(account_info->consumers, 2u);
    EXPECT_EQ(account_info->providers, 1u);
    EXPECT_EQ(account_info->sufficients, 0u);

    uint128_t free = 13'350'505'137;
    EXPECT_TRUE((uint128_t{account_info->data->free->high} << 64 |
                 account_info->data->free->low) == free);

    uint128_t reserved = 199'532'850'000'000;
    EXPECT_TRUE((uint128_t{account_info->data->reserved->high} << 64 |
                 account_info->data->reserved->low) == reserved);

    uint128_t frozen = 0;
    EXPECT_TRUE((uint128_t{account_info->data->frozen->high} << 64 |
                 account_info->data->frozen->low) == frozen);

    uint128_t flags = uint128_t{0x8000000000000000} << 64;
    EXPECT_TRUE((uint128_t{account_info->data->flags->high} << 64 |
                 account_info->data->flags->low) == flags);
  }

  {
    // Account does not exist.

    url_loader_factory_.AddResponse(
        testnet_url,
        R"({"id":1,"jsonrpc":"2.0","result":[{"block":"0x1bcd3e074b91ef25740714dc63671f4a36d2781ff93877ef9ef31b849d1ad69c","changes":[["0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9de1e86a9a8c739864cf3cc5ec2bea59fd43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d",null]]}]})");

    polkadot_substrate_rpc_->GetAccountBalance(chain_id, pubkey,
                                               future.GetCallback());

    auto [account_info, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    EXPECT_EQ(account_info->nonce, 0u);
    EXPECT_EQ(account_info->consumers, 0u);
    EXPECT_EQ(account_info->providers, 0u);
    EXPECT_EQ(account_info->sufficients, 0u);

    uint128_t free = 0;
    EXPECT_TRUE((uint128_t{account_info->data->free->high} << 64 |
                 account_info->data->free->low) == free);

    uint128_t reserved = 0;
    EXPECT_TRUE((uint128_t{account_info->data->reserved->high} << 64 |
                 account_info->data->reserved->low) == reserved);

    uint128_t frozen = 0;
    EXPECT_TRUE((uint128_t{account_info->data->frozen->high} << 64 |
                 account_info->data->frozen->low) == frozen);

    uint128_t flags = uint128_t{0x8000000000000000} << 64;
    EXPECT_TRUE((uint128_t{account_info->data->flags->high} << 64 |
                 account_info->data->flags->low) == flags);
  }

  {
    url_loader_factory_.AddResponse(
        testnet_url,
        R"({"id":1,"jsonrpc":"2.0","result":[{"block":"0x1bcd3e074b91ef25740714dc63671f4a36d2781ff93877ef9ef31b849d1ad69c","changes":[["", "0x1234"]]}]})");

    polkadot_substrate_rpc_->GetAccountBalance(chain_id, pubkey,
                                               future.GetCallback());

    auto [account_info, error] = future.Take();
    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_FALSE(account_info);
  }

  {
    url_loader_factory_.AddResponse(
        testnet_url,
        R"({"id":1,"jsonrpc":"2.0","result":[{"block":"0x1bcd3e074b91ef25740714dc63671f4a36d2781ff93877ef9ef31b849d1ad69c","changes":[]}]})");

    polkadot_substrate_rpc_->GetAccountBalance(chain_id, pubkey,
                                               future.GetCallback());

    auto [account_info, error] = future.Take();
    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_FALSE(account_info);
  }

  {
    url_loader_factory_.AddResponse(
        testnet_url,
        R"({"id":1,"jsonrpc":"2.0","result":[{"block":"0x1bcd3e074b91ef25740714dc63671f4a36d2781ff93877ef9ef31b849d1ad69c","changes":[[]]}]})");

    polkadot_substrate_rpc_->GetAccountBalance(chain_id, pubkey,
                                               future.GetCallback());

    auto [account_info, error] = future.Take();
    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_FALSE(account_info);
  }

  {
    // contains invalid hex in account info
    url_loader_factory_.AddResponse(
        testnet_url,
        R"({"id":1,"jsonrpc":"2.0","result":[{"block":"0x1bcd3e074b91ef25740714dc63671f4a36d2781ff93877ef9ef31b849d1ad69c","changes":[["", "0xcat30000020000000100000000000000b18ac01b0300000000000000000000008030a55c79b5000000000000000000000000000000000000000000000000000000000000000000000000000000000080"]]}]})");

    polkadot_substrate_rpc_->GetAccountBalance(chain_id, pubkey,
                                               future.GetCallback());

    auto [account_info, error] = future.Take();
    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_FALSE(account_info);
  }
}

}  // namespace brave_wallet
