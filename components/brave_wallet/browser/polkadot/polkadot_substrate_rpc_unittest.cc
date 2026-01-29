/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"

#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
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
    EXPECT_EQ(error, std::nullopt);
  }

  {
    // RPC node returns the wrong data type for the result.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "id": 1,
        "jsonrpc": "2.0",
        "result": 1234
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

  {
    // Chain contains no finalized head.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "id": 1,
        "jsonrpc": "2.0",
        "result": null
      })");

    polkadot_substrate_rpc_->GetFinalizedHead(chain_id, future.GetCallback());
    auto [hash, error] = future.Take();

    EXPECT_EQ(hash, std::nullopt);
    EXPECT_EQ(error, std::nullopt);
  }
}

TEST_F(PolkadotSubstrateRpcUnitTest, GetBlockHeader) {
  url_loader_factory_.ClearResponses();

  const auto* chain_id = mojom::kPolkadotTestnet;
  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");

  base::test::TestFuture<std::optional<PolkadotBlockHeader>,
                         std::optional<std::string>>
      future;

  std::optional<std::array<uint8_t, kPolkadotBlockHashSize>> parent_hash;

  {
    // Successful RPC call (nullary).

    polkadot_substrate_rpc_->GetBlockHeader(chain_id, std::nullopt,
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
        "method": "chain_getHeader",
        "params": []
      })";

    EXPECT_EQ(base::test::ParseJsonDict(
                  element.As<network::DataElementBytes>().AsStringPiece()),
              base::test::ParseJsonDict(expected_body));

    // Should match the block data here:
    // https://assethub-westend.subscan.io/block/13089907
    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "parentHash": "0xf7b0a3c2684dd0c5233b41b584faf3dded56105dea0f6d232a3432f973962b44",
          "number": "0xc7bc73",
          "stateRoot": "0x6d55a8ef28545bd8569d1c0b8f1c5bd60e30690cbb153323e69a8281a3a96d6c",
          "extrinsicsRoot": "0x0bd881aa73ac25f97052d9e34310b814a9ee500e7e04ef43940464e192234acb",
          "digest": {
            "logs": [
              "0x066175726120ac1b7f1100000000",
              "0x045250535290c612cd85d07c699b58d278616cbc9ddfe571eaab038455ee857274d0f313dc35a66bba06",
              "0x05617572610101ea4d72dd31de7db13b8a042c6d7519f059663e5a5ea6da72b6a5b7f35a8a894406e57d7577d4e64338991bb44363eab1a8f64a2c2d5109eaad1296974d4e088a"
            ]
          }
        }
      })");

    auto [header, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    EXPECT_TRUE(header);

    EXPECT_EQ(
        base::HexEncode(header->parent_hash),
        "F7B0A3C2684DD0C5233B41B584FAF3DDED56105DEA0F6D232A3432F973962B44");
    EXPECT_EQ(header->block_number, 13089907u);

    parent_hash = header->parent_hash;  // Make this available to the next test.
  }

  {
    // Successful RPC call (specific block hash provided).

    // Clear the previous responses because we run second, and also use the
    // cached parent hash from the previous test. This simulates chain-walking,
    // which is useful for interacting with the blockchain.
    url_loader_factory_.ClearResponses();
    EXPECT_TRUE(parent_hash);

    polkadot_substrate_rpc_->GetBlockHeader(chain_id, base::span(*parent_hash),
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
        "method": "chain_getHeader",
        "params": ["F7B0A3C2684DD0C5233B41B584FAF3DDED56105DEA0F6D232A3432F973962B44"]
      })";

    EXPECT_EQ(base::test::ParseJsonDict(
                  element.As<network::DataElementBytes>().AsStringPiece()),
              base::test::ParseJsonDict(expected_body));

    // Should match the block data here:
    // https://assethub-westend.subscan.io/block/13089906
    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "parentHash": "0x8c8728c828ced532d4b5785536ef426ffed39a9459f14400342e0f2b4d78c86f",
          "number": "0xc7bc72",
          "stateRoot": "0x7b65214cc5e536236b8367f07e6e4acbe124ca4a249f6c4848ee817e2348e35a",
          "extrinsicsRoot": "0xf544c1490c646fc9a4786486085781a23560fb6da1e3ca42df1491045a26a554",
          "digest": {
            "logs": [
              "0x066175726120ab1b7f1100000000",
              "0x04525053529041db728d7bcb58fab647191ba508a795f2434129c8266de0b83317d3e3bb0001a26bba06",
              "0x056175726101015827097fca69ea42dc9155f4c62220ebf2cdcf191915a497be0d35a19403937e7260444c17abb52af25f45caeb5f6117a727b4cec521e0a03d19661e2f64408b"
            ]
          }
        }
      })");

    auto [header, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    EXPECT_TRUE(header);

    EXPECT_EQ(
        base::HexEncode(header->parent_hash),
        "8C8728C828CED532D4B5785536EF426FFED39A9459F14400342E0F2B4D78C86F");
    EXPECT_EQ(header->block_number, 13089906u);
  }

  {
    // Successful RPC call (blockhash couldn't be found).

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": null
      })");

    std::array<uint8_t, kPolkadotBlockHashSize> blockhash = {};
    polkadot_substrate_rpc_->GetBlockHeader(chain_id, blockhash,
                                            future.GetCallback());

    auto [header, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    EXPECT_FALSE(header);
  }

  {
    // Error because "result" is a non-conforming value.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": 1234
      })");

    std::array<uint8_t, kPolkadotBlockHashSize> blockhash = {};
    polkadot_substrate_rpc_->GetBlockHeader(chain_id, blockhash,
                                            future.GetCallback());

    auto [header, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_FALSE(header);
  }

  {
    // Error because "result" and "error" are missing.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
      })");

    std::array<uint8_t, kPolkadotBlockHashSize> blockhash = {};
    polkadot_substrate_rpc_->GetBlockHeader(chain_id, blockhash,
                                            future.GetCallback());

    auto [header, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_FALSE(header);
  }

  {
    // RPC nodes return an error code and message.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "error": {
          "code": -32602,
          "message": "Remote failure"
        }
      })");

    std::array<uint8_t, kPolkadotBlockHashSize> blockhash = {};
    polkadot_substrate_rpc_->GetBlockHeader(chain_id, blockhash,
                                            future.GetCallback());

    auto [header, error] = future.Take();

    EXPECT_EQ(error, "Remote failure");
    EXPECT_FALSE(header);
  }

  {
    // RPC nodes return an error code.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "error": {
          "code": -32602
        }
      })");

    std::array<uint8_t, kPolkadotBlockHashSize> blockhash = {};
    polkadot_substrate_rpc_->GetBlockHeader(chain_id, blockhash,
                                            future.GetCallback());

    auto [header, error] = future.Take();

    EXPECT_EQ(error, WalletInternalErrorMessage());
    EXPECT_FALSE(header);
  }

  {
    // RPC nodes return invalid hex.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "parentHash": "0xcat728c828ced532d4b5785536ef426ffed39a9459f14400342e0f2b4d78c86f",
          "number": "0xc7bc72",
          "stateRoot": "0x7b65214cc5e536236b8367f07e6e4acbe124ca4a249f6c4848ee817e2348e35a",
          "extrinsicsRoot": "0xf544c1490c646fc9a4786486085781a23560fb6da1e3ca42df1491045a26a554",
          "digest": {
            "logs": [
              "0x066175726120ab1b7f1100000000",
              "0x04525053529041db728d7bcb58fab647191ba508a795f2434129c8266de0b83317d3e3bb0001a26bba06",
              "0x056175726101015827097fca69ea42dc9155f4c62220ebf2cdcf191915a497be0d35a19403937e7260444c17abb52af25f45caeb5f6117a727b4cec521e0a03d19661e2f64408b"
            ]
          }
        }
      })");

    std::array<uint8_t, kPolkadotBlockHashSize> blockhash = {};
    polkadot_substrate_rpc_->GetBlockHeader(chain_id, blockhash,
                                            future.GetCallback());

    auto [header, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_FALSE(header);
  }

  {
    // RPC nodes return hex that's too short.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "parentHash": "0x728c828ced532d4b5785536ef426ffed39a9459f14400342e0f2b4d78c86f",
          "number": "0xc7bc72",
          "stateRoot": "0x7b65214cc5e536236b8367f07e6e4acbe124ca4a249f6c4848ee817e2348e35a",
          "extrinsicsRoot": "0xf544c1490c646fc9a4786486085781a23560fb6da1e3ca42df1491045a26a554",
          "digest": {
            "logs": [
              "0x066175726120ab1b7f1100000000",
              "0x04525053529041db728d7bcb58fab647191ba508a795f2434129c8266de0b83317d3e3bb0001a26bba06",
              "0x056175726101015827097fca69ea42dc9155f4c62220ebf2cdcf191915a497be0d35a19403937e7260444c17abb52af25f45caeb5f6117a727b4cec521e0a03d19661e2f64408b"
            ]
          }
        }
      })");

    std::array<uint8_t, kPolkadotBlockHashSize> blockhash = {};
    polkadot_substrate_rpc_->GetBlockHeader(chain_id, blockhash,
                                            future.GetCallback());

    auto [header, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_FALSE(header);
  }

  {
    // RPC nodes return hex that's too long.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "parentHash": "0x728c828ced532d4b5785536ef426ffed39a9459f14400342e0f2b4d78c86f728c828ced532d4b5785536ef426ffed39a9459f14400342e0f2b4d78c86f",
          "number": "0xc7bc72",
          "stateRoot": "0x7b65214cc5e536236b8367f07e6e4acbe124ca4a249f6c4848ee817e2348e35a",
          "extrinsicsRoot": "0xf544c1490c646fc9a4786486085781a23560fb6da1e3ca42df1491045a26a554",
          "digest": {
            "logs": [
              "0x066175726120ab1b7f1100000000",
              "0x04525053529041db728d7bcb58fab647191ba508a795f2434129c8266de0b83317d3e3bb0001a26bba06",
              "0x056175726101015827097fca69ea42dc9155f4c62220ebf2cdcf191915a497be0d35a19403937e7260444c17abb52af25f45caeb5f6117a727b4cec521e0a03d19661e2f64408b"
            ]
          }
        }
      })");

    std::array<uint8_t, kPolkadotBlockHashSize> blockhash = {};
    polkadot_substrate_rpc_->GetBlockHeader(chain_id, blockhash,
                                            future.GetCallback());

    auto [header, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_FALSE(header);
  }

  {
    // RPC nodes return block number that exceeds numeric limits for a uint32_t.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "parentHash": "0x8c8728c828ced532d4b5785536ef426ffed39a9459f14400342e0f2b4d78c86f",
          "number": "0xc7bc72c7bc72c7bc72",
          "stateRoot": "0x7b65214cc5e536236b8367f07e6e4acbe124ca4a249f6c4848ee817e2348e35a",
          "extrinsicsRoot": "0xf544c1490c646fc9a4786486085781a23560fb6da1e3ca42df1491045a26a554",
          "digest": {
            "logs": [
              "0x066175726120ab1b7f1100000000",
              "0x04525053529041db728d7bcb58fab647191ba508a795f2434129c8266de0b83317d3e3bb0001a26bba06",
              "0x056175726101015827097fca69ea42dc9155f4c62220ebf2cdcf191915a497be0d35a19403937e7260444c17abb52af25f45caeb5f6117a727b4cec521e0a03d19661e2f64408b"
            ]
          }
        }
      })");

    std::array<uint8_t, kPolkadotBlockHashSize> blockhash = {};
    polkadot_substrate_rpc_->GetBlockHeader(chain_id, blockhash,
                                            future.GetCallback());

    auto [header, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_FALSE(header);
  }

  {
    // RPC nodes return an incomplete message, which we accept.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "parentHash": "0x8c8728c828ced532d4b5785536ef426ffed39a9459f14400342e0f2b4d78c86f",
          "number": "0xc7bc72"
        }
      })");

    std::array<uint8_t, kPolkadotBlockHashSize> blockhash = {};
    polkadot_substrate_rpc_->GetBlockHeader(chain_id, blockhash,
                                            future.GetCallback());

    auto [header, error] = future.Take();

    EXPECT_FALSE(error);
    EXPECT_EQ(
        base::HexEncode(header->parent_hash),
        "8C8728C828CED532D4B5785536EF426FFED39A9459F14400342E0F2B4D78C86F");
    EXPECT_EQ(header->block_number, 13089906u);
  }
}

TEST_F(PolkadotSubstrateRpcUnitTest, GetBlockHash) {
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
    // Successful RPC call (nullary).

    polkadot_substrate_rpc_->GetBlockHash(chain_id, std::nullopt,
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
        "method": "chain_getBlockHash",
        "params": []
      })";

    EXPECT_EQ(base::test::ParseJsonDict(
                  element.As<network::DataElementBytes>().AsStringPiece()),
              base::test::ParseJsonDict(expected_body));

    // Should match the block data here:
    // https://assethub-westend.subscan.io/block/13089907
    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": "0x4d788f8ba1a64e6cca41c047b456826f201502f5eb9b469e3f6754be1ba83564"
      })");

    auto [block_hash, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    EXPECT_EQ(
        base::HexEncode(block_hash.value_or({})),
        "4D788F8BA1A64E6CCA41C047B456826F201502F5EB9B469E3F6754BE1BA83564");
  }

  {
    // Successful RPC call (specific block number provided).

    url_loader_factory_.ClearResponses();

    polkadot_substrate_rpc_->GetBlockHash(chain_id, 13094409u,
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
        "method": "chain_getBlockHash",
        "params": ["00C7CE09"]
      })";

    EXPECT_EQ(base::test::ParseJsonDict(
                  element.As<network::DataElementBytes>().AsStringPiece()),
              base::test::ParseJsonDict(expected_body));

    // Should match the block data here:
    // https://assethub-westend.subscan.io/block/13094409
    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": "0x637fcb9534389a0ac56ae2e697655a9e73a0cd4a91d9f090c094d1f9219e5e04"
      })");

    auto [block_hash, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    EXPECT_EQ(
        base::HexEncode(block_hash.value_or({})),
        "637FCB9534389A0AC56AE2E697655A9E73A0CD4A91D9F090C094D1F9219E5E04");
  }

  {
    // Successful RPC call (genesis hash).

    url_loader_factory_.ClearResponses();

    polkadot_substrate_rpc_->GetBlockHash(chain_id, 0, future.GetCallback());

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
        "method": "chain_getBlockHash",
        "params": ["00000000"]
      })";

    EXPECT_EQ(base::test::ParseJsonDict(
                  element.As<network::DataElementBytes>().AsStringPiece()),
              base::test::ParseJsonDict(expected_body));

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": "0x67f9723393ef76214df0118c34bbbd3dbebc8ed46a10973a8c969d48fe7598c9"
      })");

    auto [block_hash, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    EXPECT_EQ(
        base::HexEncode(block_hash.value_or({})),
        "67F9723393EF76214DF0118C34BBBD3DBEBC8ED46A10973A8C969D48FE7598C9");
  }

  {
    // Successful RPC call (blockhash couldn't be found).

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": null
      })");

    polkadot_substrate_rpc_->GetBlockHash(chain_id, 1234, future.GetCallback());

    auto [block_hash, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    EXPECT_EQ(block_hash, std::nullopt);
  }

  {
    // Error because "result" is a non-conforming value.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": 1234
      })");

    polkadot_substrate_rpc_->GetBlockHash(chain_id, std::nullopt,
                                          future.GetCallback());

    auto [block_hash, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_EQ(block_hash, std::nullopt);
  }

  {
    // Error because "result" and "error" are missing.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
      })");

    polkadot_substrate_rpc_->GetBlockHash(chain_id, std::nullopt,
                                          future.GetCallback());

    auto [block_hash, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_EQ(block_hash, std::nullopt);
  }

  {
    // RPC nodes return an error code and message.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "error": {
          "code": -32602,
          "message": "Remote failure"
        }
      })");

    polkadot_substrate_rpc_->GetBlockHash(chain_id, std::nullopt,
                                          future.GetCallback());

    auto [block_hash, error] = future.Take();

    EXPECT_EQ(error, "Remote failure");
    EXPECT_EQ(block_hash, std::nullopt);
  }

  {
    // RPC nodes return an error code.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "error": {
          "code": -32602
        }
      })");

    polkadot_substrate_rpc_->GetBlockHash(chain_id, std::nullopt,
                                          future.GetCallback());

    auto [block_hash, error] = future.Take();

    EXPECT_EQ(error, WalletInternalErrorMessage());
    EXPECT_EQ(block_hash, std::nullopt);
  }

  {
    // RPC nodes return invalid hex.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": "0xcatfcb9534389a0ac56ae2e697655a9e73a0cd4a91d9f090c094d1f9219e5e04"
      })");

    polkadot_substrate_rpc_->GetBlockHash(chain_id, std::nullopt,
                                          future.GetCallback());

    auto [block_hash, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_EQ(block_hash, std::nullopt);
  }

  {
    // RPC nodes return hex that's too short.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": "0xfcb9534389a0ac56ae2e697655a9e73a0cd4a91d9f090c094d1f9219e5e04"
      })");

    polkadot_substrate_rpc_->GetBlockHash(chain_id, std::nullopt,
                                          future.GetCallback());

    auto [block_hash, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_EQ(block_hash, std::nullopt);
  }

  {
    // RPC nodes return hex that's too long.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": "0x1234637fcb9534389a0ac56ae2e697655a9e73a0cd4a91d9f090c094d1f9219e5e04"
      })");

    polkadot_substrate_rpc_->GetBlockHash(chain_id, std::nullopt,
                                          future.GetCallback());

    auto [block_hash, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_EQ(block_hash, std::nullopt);
  }
}

TEST_F(PolkadotSubstrateRpcUnitTest, GetRuntimeVersion) {
  url_loader_factory_.ClearResponses();

  const auto* chain_id = mojom::kPolkadotTestnet;
  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");

  base::test::TestFuture<std::optional<PolkadotRuntimeVersion>,
                         std::optional<std::string>>
      future;

  {
    // Successful RPC call (nullary).

    polkadot_substrate_rpc_->GetRuntimeVersion(chain_id, std::nullopt,
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
        "method": "state_getRuntimeVersion",
        "params": []
      })";

    EXPECT_EQ(base::test::ParseJsonDict(
                  element.As<network::DataElementBytes>().AsStringPiece()),
              base::test::ParseJsonDict(expected_body));

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "specName": "westend",
          "implName": "parity-westend",
          "authoringVersion": 2,
          "specVersion": 1020001,
          "implVersion": 0,
          "apis": [
            ["0xdf6acb689907609b", 5],
            ["0x37e397fc7c91f5e4", 2],
            ["0xccd9de6396c899ca", 1]
          ],
          "transactionVersion": 27,
          "systemVersion": 1,
          "stateVersion": 1
        }
      })");

    auto [runtime_version, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    EXPECT_EQ(runtime_version->spec_version, 1020001u);
    EXPECT_EQ(runtime_version->transaction_version, 27u);
  }

  {
    // Successful RPC call (specific block hash provided).

    url_loader_factory_.ClearResponses();

    const char genesis_hash[] =
        R"(0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e)";

    std::array<uint8_t, kPolkadotBlockHashSize> block_hash = {};
    EXPECT_TRUE(PrefixedHexStringToFixed(genesis_hash, block_hash));

    polkadot_substrate_rpc_->GetRuntimeVersion(chain_id, block_hash,
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
        "method": "state_getRuntimeVersion",
        "params": ["e143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e"]
      })";

    EXPECT_EQ(base::test::ParseJsonDict(
                  element.As<network::DataElementBytes>().AsStringPiece()),
              base::test::ParseJsonDict(expected_body));

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "specName": "westend",
          "implName": "parity-westend",
          "authoringVersion": 2,
          "specVersion": 1,
          "implVersion": 1,
          "apis": [
            ["0xdf6acb689907609b", 2],
            ["0x37e397fc7c91f5e4", 1]
          ],
          "transactionVersion": 1,
          "systemVersion": 0,
          "stateVersion": 0
        }
      })");

    auto [runtime_version, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    EXPECT_EQ(runtime_version->spec_version, 1u);
    EXPECT_EQ(runtime_version->transaction_version, 1u);
  }

  {
    // Failed RPC call, block hash doesn't exist.

    url_loader_factory_.ClearResponses();

    std::array<uint8_t, kPolkadotBlockHashSize> block_hash = {};
    block_hash.fill(0xff);

    polkadot_substrate_rpc_->GetRuntimeVersion(chain_id, block_hash,
                                               future.GetCallback());

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "error": {
          "code":4003,
          "message": "Client error: Api called for an unknown Block: Header was not found in the database: 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
        }
      })");

    auto [runtime_version, error] = future.Take();

    EXPECT_EQ(
        error,
        "Client error: Api called for an unknown Block: Header was not found "
        "in the database: "
        "0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    EXPECT_EQ(runtime_version, std::nullopt);
  }

  {
    // Error because result is non-conforming (bad specVersion).

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "specName": "westend",
          "implName": "parity-westend",
          "authoringVersion": 2,
          "specVersion": "hello, world!",
          "implVersion": 1,
          "apis": [
            ["0xdf6acb689907609b", 2],
            ["0x37e397fc7c91f5e4", 1]
          ],
          "transactionVersion": 1,
          "systemVersion": 0,
          "stateVersion": 0
        }
      })");

    polkadot_substrate_rpc_->GetRuntimeVersion(chain_id, std::nullopt,
                                               future.GetCallback());

    auto [runtime_version, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_EQ(runtime_version, std::nullopt);
  }

  {
    // Error because result is non-conforming (bad transactionVersion).

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "specName": "westend",
          "implName": "parity-westend",
          "authoringVersion": 2,
          "specVersion": 1,
          "implVersion": 1,
          "apis": [
            ["0xdf6acb689907609b", 2],
            ["0x37e397fc7c91f5e4", 1]
          ],
          "transactionVersion": "hello, world!!!",
          "systemVersion": 0,
          "stateVersion": 0
        }
      })");

    polkadot_substrate_rpc_->GetRuntimeVersion(chain_id, std::nullopt,
                                               future.GetCallback());

    auto [runtime_version, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_EQ(runtime_version, std::nullopt);
  }

  {
    // Error because spec and transaction version are missing.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "specName": "westend",
          "implName": "parity-westend",
          "authoringVersion": 2,
          "implVersion": 1,
          "apis": [
            ["0xdf6acb689907609b", 2],
            ["0x37e397fc7c91f5e4", 1]
          ],
          "systemVersion": 0,
          "stateVersion": 0
        }
      })");

    polkadot_substrate_rpc_->GetRuntimeVersion(chain_id, std::nullopt,
                                               future.GetCallback());

    auto [runtime_version, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_EQ(runtime_version, std::nullopt);
  }

  {
    // Error because we have no result.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "body": {}
      })");

    polkadot_substrate_rpc_->GetRuntimeVersion(chain_id, std::nullopt,
                                               future.GetCallback());

    auto [runtime_version, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_EQ(runtime_version, std::nullopt);
  }

  {
    // Error because spec version exceeds numeric limits.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "specName": "westend",
          "implName": "parity-westend",
          "authoringVersion": 2,
          "specVersion": 1234123412341234123412341234123412341234123412341234123412341234,
          "implVersion": 1,
          "apis": [
            ["0xdf6acb689907609b", 2],
            ["0x37e397fc7c91f5e4", 1]
          ],
          "transactionVersion": 1,
          "systemVersion": 0,
          "stateVersion": 0
        }
      })");

    polkadot_substrate_rpc_->GetRuntimeVersion(chain_id, std::nullopt,
                                               future.GetCallback());

    auto [runtime_version, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_EQ(runtime_version, std::nullopt);
  }

  {
    // Error because transaction version exceeds numeric limits.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "specName": "westend",
          "implName": "parity-westend",
          "authoringVersion": 2,
          "specVersion": 1,
          "implVersion": 1,
          "apis": [
            ["0xdf6acb689907609b", 2],
            ["0x37e397fc7c91f5e4", 1]
          ],
          "transactionVersion": 1234123412341234123412341234123412341234123412341234123412341234,
          "systemVersion": 0,
          "stateVersion": 0
        }
      })");

    polkadot_substrate_rpc_->GetRuntimeVersion(chain_id, std::nullopt,
                                               future.GetCallback());

    auto [runtime_version, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_EQ(runtime_version, std::nullopt);
  }

  {
    // Error because transaction version and spec version were negative.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "specName": "westend",
          "implName": "parity-westend",
          "authoringVersion": 2,
          "specVersion": -1,
          "implVersion": 1,
          "apis": [
            ["0xdf6acb689907609b", 2],
            ["0x37e397fc7c91f5e4", 1]
          ],
          "transactionVersion": -1,
          "systemVersion": 0,
          "stateVersion": 0
        }
      })");

    polkadot_substrate_rpc_->GetRuntimeVersion(chain_id, std::nullopt,
                                               future.GetCallback());

    auto [runtime_version, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_EQ(runtime_version, std::nullopt);
  }
}

TEST_F(PolkadotSubstrateRpcUnitTest, SubmitExtrinsic) {
  url_loader_factory_.ClearResponses();

  const auto* chain_id = mojom::kPolkadotTestnet;
  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");

  base::test::TestFuture<std::optional<std::string>, std::optional<std::string>>
      future;

  const char extrinsic[] =
      R"(3d02840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01349d7c183c26b13a2aba6e18fc6322c90695568b9b543f20779a9fa53d6b5d4162f5769f74b05c81aeb958d7a4be2fdc307bd1cd676ff19701f1592913995984b5012800000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4842514b00)";

  {
    // Successful RPC call, using the extrinsic submitted here:
    // https://westend.subscan.io/extrinsic/29271137-2

    polkadot_substrate_rpc_->SubmitExtrinsic(chain_id, extrinsic,
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
        "method": "author_submitExtrinsic",
        "params": ["3d02840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01349d7c183c26b13a2aba6e18fc6322c90695568b9b543f20779a9fa53d6b5d4162f5769f74b05c81aeb958d7a4be2fdc307bd1cd676ff19701f1592913995984b5012800000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4842514b00"]
      })";

    EXPECT_EQ(base::test::ParseJsonDict(
                  element.As<network::DataElementBytes>().AsStringPiece()),
              base::test::ParseJsonDict(expected_body));

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": "0x028a2de5ca3f7fd3f00a75500cc626c12ffe4347e97a00e252ac0e46a423968d"
      })");

    auto [transaction_hash, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    EXPECT_EQ(
        transaction_hash,
        "0x028a2de5ca3f7fd3f00a75500cc626c12ffe4347e97a00e252ac0e46a423968d");
  }

  {
    // Failed RPC call, extrinsic is invalid.

    url_loader_factory_.ClearResponses();

    polkadot_substrate_rpc_->SubmitExtrinsic(chain_id, extrinsic,
                                             future.GetCallback());

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "error":{
          "code": 1010,
          "message": "Invalid Transaction",
          "data":"Transaction has a bad signature"
        }
      })");

    auto [transaction_hash, error] = future.Take();

    EXPECT_EQ(error, "Invalid Transaction");
    EXPECT_EQ(transaction_hash, std::nullopt);
  }

  {
    // Error because we received something not-a-string.

    url_loader_factory_.ClearResponses();

    polkadot_substrate_rpc_->SubmitExtrinsic(chain_id, extrinsic,
                                             future.GetCallback());

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result": 1234
      })");

    auto [transaction_hash, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_EQ(transaction_hash, std::nullopt);
  }

  {
    // Error because we have no result.

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "body": {}
      })");

    polkadot_substrate_rpc_->SubmitExtrinsic(chain_id, extrinsic,
                                             future.GetCallback());

    auto [transaction_hash, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_EQ(transaction_hash, std::nullopt);
  }
}

TEST_F(PolkadotSubstrateRpcUnitTest, GetPaymentInfo) {
  url_loader_factory_.ClearResponses();

  const auto* chain_id = mojom::kPolkadotTestnet;
  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");

  std::string_view extrinsic_hex =
      R"(0x3d02840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010155034c00000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a486641f102)";

  std::vector<uint8_t> extrinsic;
  EXPECT_TRUE(PrefixedHexStringToBytes(extrinsic_hex, &extrinsic));

  {
    // Successful RPC call.

    // We cannot use TestFuture because uint128_t is too large as an individual
    // type for ToString, which TestFuture seems to require.
    auto quit_closure = task_environment_.QuitClosure();

    polkadot_substrate_rpc_->GetPaymentInfo(
        chain_id, extrinsic,
        base::BindLambdaForTesting(
            [&](base::expected<uint128_t, std::string> partial_fee) {
              EXPECT_EQ(partial_fee.value(), uint128_t{15937408476});
              quit_closure.Run();
            }));

    auto* reqs = url_loader_factory_.pending_requests();
    EXPECT_TRUE(reqs);
    EXPECT_EQ(reqs->size(), 1u);

    auto const& req = reqs->at(0);
    EXPECT_TRUE(req.request.request_body->elements());
    auto const& element = req.request.request_body->elements()->at(0);

    // The important thing to note here is that we're using state_call method
    // and that our extrinsic is the exact same as above _except_ it also has
    // the length of the extrinsic in little endian bytes append as a uint32_t.
    // i.e. 0x91000000 => 145 as this is really 0x00000091 in normal notation.
    // It is important to note that our above extrinsic is 290 hex digits (sans
    // the leading 0x), which is 145 * 2 which matches perfectly. The trailing
    // length has no need for SCALE-encoding.
    std::string_view expected_body = R"(
      {
        "id": 1,
        "jsonrpc": "2.0",
        "method": "state_call",
        "params": [
          "TransactionPaymentApi_query_info",
          "3d02840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010155034c00000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a486641f10291000000"
        ]
      })";

    EXPECT_EQ(base::test::ParseJsonDict(
                  element.As<network::DataElementBytes>().AsStringPiece()),
              base::test::ParseJsonDict(expected_body));

    url_loader_factory_.AddResponse(
        testnet_url,
        R"({"jsonrpc":"2.0","id":18,"result":"0x82ab80766da800dc8df1b5030000000000000000000000"})");

    task_environment_.RunUntilQuit();
  }

  {
    // Success, theoretical maximum fee of u128::MAX

    url_loader_factory_.ClearResponses();

    auto quit_closure = task_environment_.QuitClosure();

    polkadot_substrate_rpc_->GetPaymentInfo(
        chain_id, extrinsic,
        base::BindLambdaForTesting(
            [&](base::expected<uint128_t, std::string> partial_fee) {
              EXPECT_EQ(partial_fee.value(),
                        std::numeric_limits<uint128_t>::max());
              quit_closure.Run();
            }));

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":18,
        "result":"0x82ab80766da800ffffffffffffffffffffffffffffffff"
      })");

    task_environment_.RunUntilQuit();
  }

  {
    // Error, we typed in an incorrect destination address or one that doesn't
    // exist on the target parachain.
    // Note that the RPC nodes will give a literal dump of the Rust panic
    // running in the wasm module on the blockchain, which is an interesting bit
    // of trivia.

    url_loader_factory_.ClearResponses();

    auto quit_closure = task_environment_.QuitClosure();

    polkadot_substrate_rpc_->GetPaymentInfo(
        chain_id, extrinsic,
        base::BindLambdaForTesting(
            [&](base::expected<uint128_t, std::string> partial_fee) {
              EXPECT_EQ(partial_fee.error(), WalletParsingErrorMessage());
              quit_closure.Run();
            }));

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":18,
        "error":{
          "code":4003,
          "message":"Client error: Execution failed: Execution aborted due to trap: wasm trap: wasm `unreachable` instruction executed
          WASM backtrace:\nerror while executing at wasm backtrace:
              0: 0x9a52fc - asset_hub_westend_runtime.wasm!__rustc[4794b31dd7191200]::rust_begin_unwind
              1:   0x5211 - asset_hub_westend_runtime.wasm!core::panicking::panic_fmt::hd534225921b41838
              2: 0x90c4b5 - asset_hub_westend_runtime.wasm!TransactionPaymentApi_query_info"
        }
      })");

    task_environment_.RunUntilQuit();
  }

  {
    // Error, empty body.

    url_loader_factory_.ClearResponses();

    auto quit_closure = task_environment_.QuitClosure();

    polkadot_substrate_rpc_->GetPaymentInfo(
        chain_id, extrinsic,
        base::BindLambdaForTesting(
            [&](base::expected<uint128_t, std::string> partial_fee) {
              EXPECT_EQ(partial_fee.error(), WalletParsingErrorMessage());
              quit_closure.Run();
            }));

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":18
      })");

    task_environment_.RunUntilQuit();
  }

  {
    // Error, ref_time exceeds u64 numeric limits.
    // SCALE u128::MAX is 0x33ffffffffffffffffffffffffffffffff

    url_loader_factory_.ClearResponses();

    auto quit_closure = task_environment_.QuitClosure();

    polkadot_substrate_rpc_->GetPaymentInfo(
        chain_id, extrinsic,
        base::BindLambdaForTesting(
            [&](base::expected<uint128_t, std::string> partial_fee) {
              EXPECT_EQ(partial_fee.error(), WalletParsingErrorMessage());
              quit_closure.Run();
            }));

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":18,
        "result":"0x33ffffffffffffffffffffffffffffffff6da800dc8df1b5030000000000000000000000"
      })");

    task_environment_.RunUntilQuit();
  }

  {
    // Error, proof_size exceeds u64 numeric limits.
    // SCALE u128::MAX is 0x33ffffffffffffffffffffffffffffffff

    url_loader_factory_.ClearResponses();

    auto quit_closure = task_environment_.QuitClosure();

    polkadot_substrate_rpc_->GetPaymentInfo(
        chain_id, extrinsic,
        base::BindLambdaForTesting(
            [&](base::expected<uint128_t, std::string> partial_fee) {
              EXPECT_EQ(partial_fee.error(), WalletParsingErrorMessage());
              quit_closure.Run();
            }));

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":18,
        "result":"0x82ab807633ffffffffffffffffffffffffffffffff00dc8df1b5030000000000000000000000"
      })");

    task_environment_.RunUntilQuit();
  }

  {
    // Error, invalid class value (not 0, 1, or 2).

    url_loader_factory_.ClearResponses();

    auto quit_closure = task_environment_.QuitClosure();

    polkadot_substrate_rpc_->GetPaymentInfo(
        chain_id, extrinsic,
        base::BindLambdaForTesting(
            [&](base::expected<uint128_t, std::string> partial_fee) {
              EXPECT_EQ(partial_fee.error(), WalletParsingErrorMessage());
              quit_closure.Run();
            }));

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":18,
        "result":"0x82ab80766da8ffdc8df1b5030000000000000000000000"
      })");

    task_environment_.RunUntilQuit();
  }

  {
    // Error, fee exceeded numeric limits.

    url_loader_factory_.ClearResponses();

    auto quit_closure = task_environment_.QuitClosure();

    polkadot_substrate_rpc_->GetPaymentInfo(
        chain_id, extrinsic,
        base::BindLambdaForTesting(
            [&](base::expected<uint128_t, std::string> partial_fee) {
              EXPECT_EQ(partial_fee.error(), WalletParsingErrorMessage());
              quit_closure.Run();
            }));

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":18,
        "result":"0x82ab80766da800ffffffffffffffffffffffffffffffffffff"
      })");

    task_environment_.RunUntilQuit();
  }

  {
    // Error, message was truncated.

    url_loader_factory_.ClearResponses();

    auto quit_closure = task_environment_.QuitClosure();

    polkadot_substrate_rpc_->GetPaymentInfo(
        chain_id, extrinsic,
        base::BindLambdaForTesting(
            [&](base::expected<uint128_t, std::string> partial_fee) {
              EXPECT_EQ(partial_fee.error(), WalletParsingErrorMessage());
              quit_closure.Run();
            }));

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":18,
        "result":"0x82ab80766da800"
      })");

    task_environment_.RunUntilQuit();
  }
}

}  // namespace brave_wallet
