/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"

#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
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

  base::span<const uint8_t, kPolkadotSubstrateAccountIdSize> pubkey(out);

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

    std::string expected_body = R"(
      {
        "id": 1,
        "jsonrpc": "2.0",
        "method": "state_queryStorageAt",
        "params": [
          ["0x26AA394EEA5630E07C48AE0C9558CEF7B99D880EC681799C0CF30E8886371DA9DE1E86A9A8C739864CF3CC5EC2BEA59FD43593C715FDD31C61141ABD04A99FD6822C8558854CCDE39A5684E7A56DA27D"]
        ]
      })";

    EXPECT_EQ(base::test::ParseJsonDict(
                  element.As<network::DataElementBytes>().AsStringPiece()),
              base::test::ParseJsonDict(expected_body));

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "id": 1,
        "jsonrpc": "2.0",
        "result": [
          {
            "block": "0x1bcd3e074b91ef25740714dc63671f4a36d2781ff93877ef9ef31b849d1ad69c",
            "changes":[
              [
                "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9de1e86a9a8c739864cf3cc5ec2bea59fd43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d",
                "0x76030000020000000100000000000000b18ac01b0300000000000000000000008030a55c79b5000000000000000000000000000000000000000000000000000000000000000000000000000000000080"
              ]
            ]
          }
        ]
      })");

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

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "id":1,
        "jsonrpc":"2.0",
        "result":[
          {
            "block": "0x1bcd3e074b91ef25740714dc63671f4a36d2781ff93877ef9ef31b849d1ad69c",
            "changes":[
              ["0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9de1e86a9a8c739864cf3cc5ec2bea59fd43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d", null]
            ]
          }
        ]
      })");

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
    // Account data is too short.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "id":1,
        "jsonrpc":"2.0",
        "result":[
          {
            "block":""0x1bcd3e074b91ef25740714dc63671f4a36d2781ff93877ef9ef31b849d1ad69c",
            "changes":[["", "0x1234"]]
          }
        ]
      })");

    polkadot_substrate_rpc_->GetAccountBalance(chain_id, pubkey,
                                               future.GetCallback());

    auto [account_info, error] = future.Take();
    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_FALSE(account_info);
  }

  {
    // Changes array is empty.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "id":1,
        "jsonrpc":"2.0",
        "result":[
          {
            "block":"0x1bcd3e074b91ef25740714dc63671f4a36d2781ff93877ef9ef31b849d1ad69c",
            "changes":[]
          }
        ]
      })");

    polkadot_substrate_rpc_->GetAccountBalance(chain_id, pubkey,
                                               future.GetCallback());

    auto [account_info, error] = future.Take();
    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_FALSE(account_info);
  }

  {
    // Changes array contains empty pair (no storage key, no account
    // information).

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "id":1,
        "jsonrpc":"2.0",
        "result":[
          {
            "block": "0x1bcd3e074b91ef25740714dc63671f4a36d2781ff93877ef9ef31b849d1ad69c",
            "changes":[[]]
          }
        ]
      })");

    polkadot_substrate_rpc_->GetAccountBalance(chain_id, pubkey,
                                               future.GetCallback());

    auto [account_info, error] = future.Take();
    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_FALSE(account_info);
  }

  {
    // Contains invalid hex in account info.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "id":1,
        "jsonrpc":"2.0",
        "result":[
          {
            "block":"0x1bcd3e074b91ef25740714dc63671f4a36d2781ff93877ef9ef31b849d1ad69c",
            "changes":[
              ["", "0xcat30000020000000100000000000000b18ac01b0300000000000000000000008030a55c79b5000000000000000000000000000000000000000000000000000000000000000000000000000000000080"]
            ]
          }
        ]
      })");

    polkadot_substrate_rpc_->GetAccountBalance(chain_id, pubkey,
                                               future.GetCallback());

    auto [account_info, error] = future.Take();
    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_FALSE(account_info);
  }

  {
    // Server contained invalid response.

    url_loader_factory_.AddResponse(
        testnet_url, "some invalid data goes here",
        net::HttpStatusCode::HTTP_INTERNAL_SERVER_ERROR);

    polkadot_substrate_rpc_->GetAccountBalance(chain_id, pubkey,
                                               future.GetCallback());

    auto [account_info, error] = future.Take();
    EXPECT_EQ(error, WalletInternalErrorMessage());
    EXPECT_FALSE(account_info);
  }

  {
    // Numeric limits.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "id":1,
        "jsonrpc":"2.0",
        "result":[
          {
            "block":"0x1bcd3e074b91ef25740714dc63671f4a36d2781ff93877ef9ef31b849d1ad69c",
            "changes":[
              [
                "0x26aa394eea5630e07c48ae0c9558cef7b99d880ec681799c0cf30e8886371da9de1e86a9a8c739864cf3cc5ec2bea59fd43593c715fdd31c61141abd04a99fd6822c8558854ccde39a5684e7a56da27d",
                "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
              ]
            ]
          }
        ]
      })");

    polkadot_substrate_rpc_->GetAccountBalance(chain_id, pubkey,
                                               future.GetCallback());

    auto [account_info, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    EXPECT_EQ(account_info->nonce, std::numeric_limits<uint32_t>::max());
    EXPECT_EQ(account_info->consumers, std::numeric_limits<uint32_t>::max());
    EXPECT_EQ(account_info->providers, std::numeric_limits<uint32_t>::max());
    EXPECT_EQ(account_info->sufficients, std::numeric_limits<uint32_t>::max());

    uint128_t free = std::numeric_limits<uint128_t>::max();
    EXPECT_TRUE((uint128_t{account_info->data->free->high} << 64 |
                 account_info->data->free->low) == free);

    uint128_t reserved = std::numeric_limits<uint128_t>::max();

    EXPECT_TRUE((uint128_t{account_info->data->reserved->high} << 64 |
                 account_info->data->reserved->low) == reserved);

    uint128_t frozen = std::numeric_limits<uint128_t>::max();
    EXPECT_TRUE((uint128_t{account_info->data->frozen->high} << 64 |
                 account_info->data->frozen->low) == frozen);

    uint128_t flags = std::numeric_limits<uint128_t>::max();
    EXPECT_TRUE((uint128_t{account_info->data->flags->high} << 64 |
                 account_info->data->flags->low) == flags);
  }
}

TEST_F(PolkadotSubstrateRpcUnitTest, GetFinalizedHead) {
  url_loader_factory_.ClearResponses();

  const auto* chain_id = mojom::kPolkadotTestnet;
  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");

  base::test::TestFuture<
      std::optional<std::array<uint8_t, kPolkadotBlockHashSize>>,
      std::optional<std::string>>
      future;

  {
    // Successful RPC call.

    polkadot_substrate_rpc_->GetFinalizedHead(chain_id, future.GetCallback());

    auto* reqs = url_loader_factory_.pending_requests();
    EXPECT_TRUE(reqs);
    EXPECT_EQ(reqs->size(), 1u);

    auto const& req = reqs->at(0);
    EXPECT_TRUE(req.request.request_body->elements());
    auto const& element = req.request.request_body->elements()->at(0);

    std::string expected_body = R"(
      {
        "id": 1,
        "jsonrpc": "2.0",
        "method": "chain_getFinalizedHead",
        "params": []
      })";

    EXPECT_EQ(base::test::ParseJsonDict(
                  element.As<network::DataElementBytes>().AsStringPiece()),
              base::test::ParseJsonDict(expected_body));

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "id": 1,
        "jsonrpc": "2.0",
        "result":"0xba38d3e0e1033e97a3aa294e59741c9f4ab8786c8d55c493d0ebc58b885961b3"
      })");

    auto [hash, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    EXPECT_EQ(
        base::HexEncode(hash.value()),
        "BA38D3E0E1033E97A3AA294E59741C9F4AB8786C8D55C493D0EBC58B885961B3");
  }

  {
    // RPC node returns an error code, with a message.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "id": 1,
        "jsonrpc": "2.0",
        "error": {
          "code": -32700,
          "message": "Network outage"
        }
      })");

    polkadot_substrate_rpc_->GetFinalizedHead(chain_id, future.GetCallback());
    auto [hash, error] = future.Take();

    EXPECT_EQ(hash, std::nullopt);
    EXPECT_EQ(error, "Network outage");
  }

  {
    // RPC node returns an error code, with no message.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "id": 1,
        "jsonrpc": "2.0",
        "error": {
          "code": -32700
        }
      })");

    polkadot_substrate_rpc_->GetFinalizedHead(chain_id, future.GetCallback());
    auto [hash, error] = future.Take();

    EXPECT_EQ(hash, std::nullopt);
    EXPECT_EQ(error, WalletInternalErrorMessage());
  }

  {
    // RPC node returns something non-compliant.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "id": 1,
        "jsonrpc": "2.0",
        "data": "random stuff"
      })");

    polkadot_substrate_rpc_->GetFinalizedHead(chain_id, future.GetCallback());
    auto [hash, error] = future.Take();

    EXPECT_EQ(hash, std::nullopt);
    EXPECT_EQ(error, WalletParsingErrorMessage());
  }

  {
    // RPC node returns the wrong data type for the result.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "id": 1,
        "jsonrpc": "2.0",
        "data": 1234
      })");

    polkadot_substrate_rpc_->GetFinalizedHead(chain_id, future.GetCallback());
    auto [hash, error] = future.Take();

    EXPECT_EQ(hash, std::nullopt);
    EXPECT_EQ(error, WalletParsingErrorMessage());
  }

  {
    // RPC node returns invalid hex string.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "id": 1,
        "jsonrpc": "2.0",
        "result": "0xcat1234",
      })");

    polkadot_substrate_rpc_->GetFinalizedHead(chain_id, future.GetCallback());
    auto [hash, error] = future.Take();

    EXPECT_EQ(hash, std::nullopt);
    EXPECT_EQ(error, WalletParsingErrorMessage());
  }
}

}  // namespace brave_wallet
