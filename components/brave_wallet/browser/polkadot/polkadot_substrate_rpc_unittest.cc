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
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  ~PolkadotSubstrateRpcUnitTest() override = default;

  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    RegisterLocalStatePrefs(local_state_.registry());

    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
    polkadot_substrate_rpc_ = std::make_unique<PolkadotSubstrateRpc>(
        *network_manager_, url_loader_factory_.GetSafeWeakWrapper());
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  sync_preferences::TestingPrefServiceSyncable local_state_;

  network::TestURLLoaderFactory url_loader_factory_;
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<PolkadotSubstrateRpc> polkadot_substrate_rpc_;
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
        base::HexEncodeLower(header->parent_hash),
        "f7b0a3c2684dd0c5233b41b584faf3dded56105dea0f6d232a3432f973962b44");
    EXPECT_EQ(header->block_number, 13089907u);
    EXPECT_EQ(
        base::HexEncodeLower(header->state_root),
        "6d55a8ef28545bd8569d1c0b8f1c5bd60e30690cbb153323e69a8281a3a96d6c");
    EXPECT_EQ(
        base::HexEncodeLower(header->extrinsics_root),
        "0bd881aa73ac25f97052d9e34310b814a9ee500e7e04ef43940464e192234acb");

    EXPECT_EQ(
        base::HexEncodeLower(header->encoded_logs),
        "0c"
        R"(066175726120ac1b7f1100000000)"
        R"(045250535290c612cd85d07c699b58d278616cbc9ddfe571eaab038455ee857274d0f313dc35a66bba06)"
        R"(05617572610101ea4d72dd31de7db13b8a042c6d7519f059663e5a5ea6da72b6a5b7f35a8a894406e57d7577d4e64338991bb44363eab1a8f64a2c2d5109eaad1296974d4e088a)");

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
        "params": ["f7b0a3c2684dd0c5233b41b584faf3dded56105dea0f6d232a3432f973962b44"]
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
        base::HexEncodeLower(header->parent_hash),
        "8c8728c828ced532d4b5785536ef426ffed39a9459f14400342e0f2b4d78c86f");
    EXPECT_EQ(header->block_number, 13089906u);
    EXPECT_EQ(
        base::HexEncodeLower(header->state_root),
        "7b65214cc5e536236b8367f07e6e4acbe124ca4a249f6c4848ee817e2348e35a");
    EXPECT_EQ(
        base::HexEncodeLower(header->extrinsics_root),
        "f544c1490c646fc9a4786486085781a23560fb6da1e3ca42df1491045a26a554");

    EXPECT_EQ(
        base::HexEncodeLower(header->encoded_logs),
        "0c"
        R"(066175726120ab1b7f1100000000)"
        R"(04525053529041db728d7bcb58fab647191ba508a795f2434129c8266de0b83317d3e3bb0001a26bba06)"
        R"(056175726101015827097fca69ea42dc9155f4c62220ebf2cdcf191915a497be0d35a19403937e7260444c17abb52af25f45caeb5f6117a727b4cec521e0a03d19661e2f64408b)");
  }

  {
    // Successful RPC call (genesis block).
    url_loader_factory_.AddResponse(testnet_url, R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "parentHash": "0x0000000000000000000000000000000000000000000000000000000000000000",
          "number": "0x0",
          "stateRoot": "0x7e92439a94f79671f9cade9dff96a094519b9001a7432244d46ab644bb6f746f",
          "extrinsicsRoot": "0x03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314",
          "digest": {
            "logs": []
          }
        }
      })");

    std::array<uint8_t, kPolkadotBlockHashSize> genesis_hash = {};
    EXPECT_TRUE(PrefixedHexStringToFixed(
        "0xe143f23803ac50e8f6f8e62695d1ce9e4e1d68aa36c1cd2cfd15340213f3423e",
        genesis_hash));
    polkadot_substrate_rpc_->GetBlockHeader(chain_id, genesis_hash,
                                            future.GetCallback());

    auto [header, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    EXPECT_TRUE(header);

    EXPECT_EQ(
        base::HexEncodeLower(header->parent_hash),
        "0000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(header->block_number, 0u);
    EXPECT_EQ(
        base::HexEncodeLower(header->state_root),
        "7e92439a94f79671f9cade9dff96a094519b9001a7432244d46ab644bb6f746f");
    EXPECT_EQ(
        base::HexEncodeLower(header->extrinsics_root),
        "03170a2e7597b7b7e3d84c05391d139a62b157e78786d8c082f29dcf4c111314");

    EXPECT_EQ(base::HexEncodeLower(header->encoded_logs), "00");
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
    // RPC nodes return hex that's too short (parent hash).

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
    // RPC nodes return hex that's too long (parent hash).

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
    // RPC nodes return an incomplete message, which we no longer accept.

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

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_FALSE(header);
  }

  {
    // RPC nodes return hex that's too long (state root).

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "parentHash": "0x8c8728c828ced532d4b5785536ef426ffed39a9459f14400342e0f2b4d78c86f",
          "number": "0xc7bc72",
          "stateRoot": "0x7b65214cc5e536236b8367f07e6e4acbe124ca4a249f6c4848ee817e2348e35aff",
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
    // RPC nodes return hex that's too long (extrinsics root).

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc": "2.0",
        "id": 1,
        "result": {
          "parentHash": "0x8c8728c828ced532d4b5785536ef426ffed39a9459f14400342e0f2b4d78c86f",
          "number": "0xc7bc72",
          "stateRoot": "0x7b65214cc5e536236b8367f07e6e4acbe124ca4a249f6c4848ee817e2348e35a",
          "extrinsicsRoot": "0xf544c1490c646fc9a4786486085781a23560fb6da1e3ca42df1491045a26a554ff",
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
    // RPC nodes digest logs that contain non-hex (no length minimum on logs).

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
              "0x066175726120ab1b7f1100000000cat",
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
    // RPC nodes return invalid digest.

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
          "digest": 1234
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
    // RPC nodes return invalid logs.

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
            "logs": 1234
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

TEST_F(PolkadotSubstrateRpcUnitTest, GetBlock) {
  // The routines that parse blocks should reuse the routines that already parse
  // the block headers, which means this test in general needs a much less heavy
  // testing burden. All we really need to do here is make sure that we get an
  // array of hex strings.

  const auto* chain_id = mojom::kPolkadotTestnet;
  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");

  base::test::TestFuture<std::optional<PolkadotBlock>,
                         std::optional<std::string>>
      future;

  {
    // Successful RPC call (nullary).

    url_loader_factory_.ClearResponses();

    polkadot_substrate_rpc_->GetBlock(chain_id, std::nullopt,
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
        "method": "chain_getBlock",
        "params": []
      })";

    EXPECT_EQ(base::test::ParseJsonDict(
                  element.As<network::DataElementBytes>().AsStringPiece()),
              base::test::ParseJsonDict(expected_body));

    // Should match the block data here:
    // https://westend.subscan.io/block/29986674
    // Note that we choose to exclude one of the extrinsics from the test data
    // because it's simply just too large, spanning tens of KBs worth of hex
    // data which is absurd for a unit test.
    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "block":{
            "header":{
              "parentHash":"0x679bf2ce8ad7a75903b61d53c3fef2ce52fa01af7ee25dbd7a82a3516756dc61",
              "number":"0x1c98f72",
              "stateRoot":"0xbbc2fd81afd18ac40c31ba2f909da0b228e1d505ceaf79f17a40b2dd964f0c55",
              "extrinsicsRoot":"0x9d1107a618c83b48f26e9af67a4df58565ee1905f01bee582903b67d528e7e68",
              "digest":{
                "logs":[
                  "0x0642414245b5010104000000bcbf9a110000000078c84652fec3ebd75549aeabd0cbc84f9232606cef56aa80e304e0adffe8db69c58956fb12435293cad94e61ea55d6dbbd6edab1ca4aa10ee46f8ded4bec79094369c343adf1b4d76d03554901d8e79e3e3d92c5824348731cf1f3d142e10c09",
                  "0x04424545468403be55687de385553673894b02babee86a09ab5da4cd569eb8a2e10acb60e05e27",
                  "0x05424142450101aa2ef2a5c8b8f8d1119437e40c15b30fbe13a7ab56e86fa90a729d166a74426c8d35e9c17ab858e922c913da0f2af2ebda3a1a41172884022ed8ab5cfba3e189"
                ]
              }
            },
            "extrinsics":[
              "0x280502000b41c6ed9a9c01",
              "0x4502840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01fe7084bd98bd4c8cdee53ffcacc642d4647d6dac32824a1674e9b8883ea61a3870696b0c07363f482183e615c7a55f8a66cde7eb7bc11e1242001527919dde8ef5027000000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4807009236bb1c"
            ]
          },
          "justifications":null
        }
      })");

    auto [block, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    ASSERT_TRUE(block);

    EXPECT_EQ(
        base::HexEncodeLower(block->header.parent_hash),
        "679bf2ce8ad7a75903b61d53c3fef2ce52fa01af7ee25dbd7a82a3516756dc61");
    EXPECT_EQ(block->header.block_number, 29986674u);
    EXPECT_EQ(
        base::HexEncodeLower(block->header.state_root),
        "bbc2fd81afd18ac40c31ba2f909da0b228e1d505ceaf79f17a40b2dd964f0c55");
    EXPECT_EQ(
        base::HexEncodeLower(block->header.extrinsics_root),
        "9d1107a618c83b48f26e9af67a4df58565ee1905f01bee582903b67d528e7e68");

    EXPECT_EQ(
        base::HexEncodeLower(block->header.encoded_logs),
        "0c"
        R"(0642414245b5010104000000bcbf9a110000000078c84652fec3ebd75549aeabd0cbc84f9232606cef56aa80e304e0adffe8db69c58956fb12435293cad94e61ea55d6dbbd6edab1ca4aa10ee46f8ded4bec79094369c343adf1b4d76d03554901d8e79e3e3d92c5824348731cf1f3d142e10c09)"
        R"(04424545468403be55687de385553673894b02babee86a09ab5da4cd569eb8a2e10acb60e05e27)"
        R"(05424142450101aa2ef2a5c8b8f8d1119437e40c15b30fbe13a7ab56e86fa90a729d166a74426c8d35e9c17ab858e922c913da0f2af2ebda3a1a41172884022ed8ab5cfba3e189)");

    EXPECT_THAT(
        block->extrinsics,
        testing::ElementsAre(
            R"(0x280502000b41c6ed9a9c01)",
            R"(0x4502840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01fe7084bd98bd4c8cdee53ffcacc642d4647d6dac32824a1674e9b8883ea61a3870696b0c07363f482183e615c7a55f8a66cde7eb7bc11e1242001527919dde8ef5027000000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4807009236bb1c)"));

    EXPECT_EQ(
        base::HexEncodeLower(block->header.GetHash()),
        "e1cf95144139c7b92223a5bf20106b97b350c3803f4b0a11e14d38a6de758711");
  }

  {
    // Successful RPC call (block hash provided).

    url_loader_factory_.ClearResponses();

    std::array<uint8_t, kPolkadotBlockHashSize> block_hash = {};
    ASSERT_TRUE(base::HexStringToSpan(
        "e1cf95144139c7b92223a5bf20106b97b350c3803f4b0a11e14d38a6de758711",
        block_hash));

    polkadot_substrate_rpc_->GetBlock(chain_id, block_hash,
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
        "method": "chain_getBlock",
        "params": ["e1cf95144139c7b92223a5bf20106b97b350c3803f4b0a11e14d38a6de758711"]
      })";

    EXPECT_EQ(base::test::ParseJsonDict(
                  element.As<network::DataElementBytes>().AsStringPiece()),
              base::test::ParseJsonDict(expected_body));

    // Should match the block data here:
    // https://westend.subscan.io/block/29986674
    // Note that we choose to exclude one of the extrinsics from the test data
    // because it's simply just too large, spanning tens of KBs worth of hex
    // data which is absurd for a unit test.
    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "block":{
            "header":{
              "parentHash":"0x679bf2ce8ad7a75903b61d53c3fef2ce52fa01af7ee25dbd7a82a3516756dc61",
              "number":"0x1c98f72",
              "stateRoot":"0xbbc2fd81afd18ac40c31ba2f909da0b228e1d505ceaf79f17a40b2dd964f0c55",
              "extrinsicsRoot":"0x9d1107a618c83b48f26e9af67a4df58565ee1905f01bee582903b67d528e7e68",
              "digest":{
                "logs":[
                  "0x0642414245b5010104000000bcbf9a110000000078c84652fec3ebd75549aeabd0cbc84f9232606cef56aa80e304e0adffe8db69c58956fb12435293cad94e61ea55d6dbbd6edab1ca4aa10ee46f8ded4bec79094369c343adf1b4d76d03554901d8e79e3e3d92c5824348731cf1f3d142e10c09",
                  "0x04424545468403be55687de385553673894b02babee86a09ab5da4cd569eb8a2e10acb60e05e27",
                  "0x05424142450101aa2ef2a5c8b8f8d1119437e40c15b30fbe13a7ab56e86fa90a729d166a74426c8d35e9c17ab858e922c913da0f2af2ebda3a1a41172884022ed8ab5cfba3e189"
                ]
              }
            },
            "extrinsics":[
              "0x280502000b41c6ed9a9c01",
              "0x4502840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01fe7084bd98bd4c8cdee53ffcacc642d4647d6dac32824a1674e9b8883ea61a3870696b0c07363f482183e615c7a55f8a66cde7eb7bc11e1242001527919dde8ef5027000000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4807009236bb1c"
            ]
          },
          "justifications":null
        }
      })");

    auto [block, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    ASSERT_TRUE(block);

    EXPECT_EQ(
        base::HexEncodeLower(block->header.parent_hash),
        "679bf2ce8ad7a75903b61d53c3fef2ce52fa01af7ee25dbd7a82a3516756dc61");
    EXPECT_EQ(block->header.block_number, 29986674u);
    EXPECT_EQ(
        base::HexEncodeLower(block->header.state_root),
        "bbc2fd81afd18ac40c31ba2f909da0b228e1d505ceaf79f17a40b2dd964f0c55");
    EXPECT_EQ(
        base::HexEncodeLower(block->header.extrinsics_root),
        "9d1107a618c83b48f26e9af67a4df58565ee1905f01bee582903b67d528e7e68");

    EXPECT_EQ(
        base::HexEncodeLower(block->header.encoded_logs),
        "0c"
        R"(0642414245b5010104000000bcbf9a110000000078c84652fec3ebd75549aeabd0cbc84f9232606cef56aa80e304e0adffe8db69c58956fb12435293cad94e61ea55d6dbbd6edab1ca4aa10ee46f8ded4bec79094369c343adf1b4d76d03554901d8e79e3e3d92c5824348731cf1f3d142e10c09)"
        R"(04424545468403be55687de385553673894b02babee86a09ab5da4cd569eb8a2e10acb60e05e27)"
        R"(05424142450101aa2ef2a5c8b8f8d1119437e40c15b30fbe13a7ab56e86fa90a729d166a74426c8d35e9c17ab858e922c913da0f2af2ebda3a1a41172884022ed8ab5cfba3e189)");

    EXPECT_THAT(
        block->extrinsics,
        testing::ElementsAre(
            R"(0x280502000b41c6ed9a9c01)",
            R"(0x4502840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01fe7084bd98bd4c8cdee53ffcacc642d4647d6dac32824a1674e9b8883ea61a3870696b0c07363f482183e615c7a55f8a66cde7eb7bc11e1242001527919dde8ef5027000000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4807009236bb1c)"));

    EXPECT_EQ(
        base::HexEncodeLower(block->header.GetHash()),
        "e1cf95144139c7b92223a5bf20106b97b350c3803f4b0a11e14d38a6de758711");
  }

  {
    // Successful RPC call (can't be found).

    url_loader_factory_.ClearResponses();

    std::array<uint8_t, kPolkadotBlockHashSize> block_hash = {};
    ASSERT_TRUE(base::HexStringToSpan(
        "e1cf95144139c7b92223a5bf20106b97b350c3803f4b0a11e14d38a6de758712",
        block_hash));

    polkadot_substrate_rpc_->GetBlock(chain_id, block_hash,
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
        "method": "chain_getBlock",
        "params": ["e1cf95144139c7b92223a5bf20106b97b350c3803f4b0a11e14d38a6de758712"]
      })";

    EXPECT_EQ(base::test::ParseJsonDict(
                  element.As<network::DataElementBytes>().AsStringPiece()),
              base::test::ParseJsonDict(expected_body));

    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result":null
      })");

    auto [block, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    EXPECT_EQ(block, std::nullopt);
  }

  {
    // Remote returned invalid hex in an extrinsic.

    url_loader_factory_.ClearResponses();

    std::array<uint8_t, kPolkadotBlockHashSize> block_hash = {};
    ASSERT_TRUE(base::HexStringToSpan(
        "e1cf95144139c7b92223a5bf20106b97b350c3803f4b0a11e14d38a6de758712",
        block_hash));

    polkadot_substrate_rpc_->GetBlock(chain_id, block_hash,
                                      future.GetCallback());
    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "block":{
            "header":{
              "parentHash":"0x679bf2ce8ad7a75903b61d53c3fef2ce52fa01af7ee25dbd7a82a3516756dc61",
              "number":"0x1c98f72",
              "stateRoot":"0xbbc2fd81afd18ac40c31ba2f909da0b228e1d505ceaf79f17a40b2dd964f0c55",
              "extrinsicsRoot":"0x9d1107a618c83b48f26e9af67a4df58565ee1905f01bee582903b67d528e7e68",
              "digest":{
                "logs":[
                  "0x0642414245b5010104000000bcbf9a110000000078c84652fec3ebd75549aeabd0cbc84f9232606cef56aa80e304e0adffe8db69c58956fb12435293cad94e61ea55d6dbbd6edab1ca4aa10ee46f8ded4bec79094369c343adf1b4d76d03554901d8e79e3e3d92c5824348731cf1f3d142e10c09",
                  "0x04424545468403be55687de385553673894b02babee86a09ab5da4cd569eb8a2e10acb60e05e27",
                  "0x05424142450101aa2ef2a5c8b8f8d1119437e40c15b30fbe13a7ab56e86fa90a729d166a74426c8d35e9c17ab858e922c913da0f2af2ebda3a1a41172884022ed8ab5cfba3e189"
                ]
              }
            },
            "extrinsics":[
              "0xcat280502000b41c6ed9a9c01",
              "0x4502840052707850d9298f5dfb0a3e5b23fcca39ea286c6def2db5716c996fb39db6477c01fe7084bd98bd4c8cdee53ffcacc642d4647d6dac32824a1674e9b8883ea61a3870696b0c07363f482183e615c7a55f8a66cde7eb7bc11e1242001527919dde8ef5027000000400008eaf04151687736326c9fea17e25fc5287613693c912909cb226aa4794f26a4807009236bb1c"
            ]
          },
          "justifications":null
        }
      })");

    auto [block, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_EQ(block, std::nullopt);
  }

  {
    // Remote sent back an empty extrinsics array.

    url_loader_factory_.ClearResponses();

    polkadot_substrate_rpc_->GetBlock(chain_id, std::nullopt,
                                      future.GetCallback());
    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "block":{
            "header":{
              "parentHash":"0x679bf2ce8ad7a75903b61d53c3fef2ce52fa01af7ee25dbd7a82a3516756dc61",
              "number":"0x1c98f72",
              "stateRoot":"0xbbc2fd81afd18ac40c31ba2f909da0b228e1d505ceaf79f17a40b2dd964f0c55",
              "extrinsicsRoot":"0x9d1107a618c83b48f26e9af67a4df58565ee1905f01bee582903b67d528e7e68",
              "digest":{
                "logs":[
                  "0x0642414245b5010104000000bcbf9a110000000078c84652fec3ebd75549aeabd0cbc84f9232606cef56aa80e304e0adffe8db69c58956fb12435293cad94e61ea55d6dbbd6edab1ca4aa10ee46f8ded4bec79094369c343adf1b4d76d03554901d8e79e3e3d92c5824348731cf1f3d142e10c09",
                  "0x04424545468403be55687de385553673894b02babee86a09ab5da4cd569eb8a2e10acb60e05e27",
                  "0x05424142450101aa2ef2a5c8b8f8d1119437e40c15b30fbe13a7ab56e86fa90a729d166a74426c8d35e9c17ab858e922c913da0f2af2ebda3a1a41172884022ed8ab5cfba3e189"
                ]
              }
            },
            "extrinsics":[]
          },
          "justifications":null
        }
      })");

    auto [block, error] = future.Take();

    EXPECT_EQ(error, std::nullopt);
    ASSERT_TRUE(block);
    EXPECT_TRUE(block->extrinsics.empty());
  }

  {
    // Extrinsics field is a non-conforming type (number instead of array).

    url_loader_factory_.ClearResponses();

    polkadot_substrate_rpc_->GetBlock(chain_id, std::nullopt,
                                      future.GetCallback());
    url_loader_factory_.AddResponse(testnet_url,
                                    R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result":{
          "block":{
            "header":{
              "parentHash":"0x679bf2ce8ad7a75903b61d53c3fef2ce52fa01af7ee25dbd7a82a3516756dc61",
              "number":"0x1c98f72",
              "stateRoot":"0xbbc2fd81afd18ac40c31ba2f909da0b228e1d505ceaf79f17a40b2dd964f0c55",
              "extrinsicsRoot":"0x9d1107a618c83b48f26e9af67a4df58565ee1905f01bee582903b67d528e7e68",
              "digest":{
                "logs":[
                  "0x0642414245b5010104000000bcbf9a110000000078c84652fec3ebd75549aeabd0cbc84f9232606cef56aa80e304e0adffe8db69c58956fb12435293cad94e61ea55d6dbbd6edab1ca4aa10ee46f8ded4bec79094369c343adf1b4d76d03554901d8e79e3e3d92c5824348731cf1f3d142e10c09",
                  "0x04424545468403be55687de385553673894b02babee86a09ab5da4cd569eb8a2e10acb60e05e27",
                  "0x05424142450101aa2ef2a5c8b8f8d1119437e40c15b30fbe13a7ab56e86fa90a729d166a74426c8d35e9c17ab858e922c913da0f2af2ebda3a1a41172884022ed8ab5cfba3e189"
                ]
              }
            },
            "extrinsics": 42
          },
          "justifications":null
        }
      })");

    auto [block, error] = future.Take();

    EXPECT_EQ(error, WalletParsingErrorMessage());
    EXPECT_EQ(block, std::nullopt);
  }
}

TEST_F(PolkadotSubstrateRpcUnitTest, GetEvents) {
  const auto* chain_id = mojom::kPolkadotTestnet;
  std::string testnet_url =
      network_manager_
          ->GetKnownChain(mojom::kPolkadotTestnet, mojom::CoinType::DOT)
          ->rpc_endpoints.front()
          .spec();

  EXPECT_EQ(testnet_url, "https://polkadot-westend.wallet.brave.com/");

  constexpr const char kBlockHashHex[] =
      "240ecc246eaeea2f75c69fde3d4ce1a7f9761eeaf0a37a9b5e0458f4da9e2819";

  std::array<uint8_t, kPolkadotBlockHashSize> block_hash;
  EXPECT_TRUE(base::HexStringToSpan(kBlockHashHex, block_hash));

  base::test::TestFuture<base::expected<std::vector<uint8_t>, std::string>>
      future;

  {
    // Successful RPC call.

    url_loader_factory_.ClearResponses();

    polkadot_substrate_rpc_->GetEvents(chain_id, block_hash,
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
        "method": "state_getStorage",
        "params": [
          "26aa394eea5630e07c48ae0c9558cef780d41e5e16056765bc8461851072c9d7",
          "240ecc246eaeea2f75c69fde3d4ce1a7f9761eeaf0a37a9b5e0458f4da9e2819"
        ]
      })";

    EXPECT_EQ(base::test::ParseJsonDict(
                  element.As<network::DataElementBytes>().AsStringPiece()),
              base::test::ParseJsonDict(expected_body));

    std::string expected_res =
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
        "\"0x30000000000000006279e941551702000000010000002c01b8090000d3a927d4b1"
        "547a0d854584a84b68e382ff246d85b041f2212b53a4280f1e80ad000800fcc9000000"
        "0000000000000000000000000000000000000000000000000136d94c8625f07fcd5751"
        "b8525e281839bc25bfa33e1c746515e1bf2cbfe5826804fedc243a504533f5cfece0fd"
        "25bea75a0f50b0243d750cb3a3f60b3164e7c6871543662eb3e7d4f9ffdd329ccfd646"
        "6d8099b7ca5f7b5dc4d36077b338e80000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000f3760c1e7536ab5e9451464f83f27a3102851d4acf8a2170db19"
        "823a77a76c799cd302e2215a071302a7b64f00d9cd2e14a6b48e026349599ebaf7bc5f"
        "d275153c01b3c63d6a5ef79e6caf27ad06aafb57153dce0a97ec3473f326d804a668b8"
        "b90304673bd1205ddc9b8f2968f51adfbd14563ced437bd59e8171ca7eb0985cb2105a"
        "63030010c58992e3a0179c4c1171b4b09a752c51012bd5cdb89fa1af365742f4ff4b01"
        "a53cf259d5532918399624a761352504336d1f5f8c69bb58e5787e8f1bfbd6a6100643"
        "4d4c5310010001040661757261201e95ce08000000000452505352909967accd39b9c4"
        "ca2c1079e7006681eb62dfc9e5e95d04e851ab85fb7ae415dacee72f07056175726101"
        "01761dbe7d86c140b508cd6d672ecc8c7fd086f39290d45bbf4dd81f731d371664aa4a"
        "5c89abe4cc8ff5cbb66c80a6023201ec742a0889fa880f04f5c486f8fc89080000000f"
        "0000000000010000002c01e9030000d3a927d4b1547a0d854584a84b68e382ff246d85"
        "b041f2212b53a4280f1e80ad000100fcc9000000000000000000000000000000000000"
        "000000000000000000eac4b57e3173e66431c95399cd18aefaa05335b7347497419372"
        "56d36a5def1b048236d1541171924dfca308072d5604a8c4deaafcb9b481d93ea94c62"
        "bc52562d8373145dc6d3d72378eb18fd00bfea1f52f6420453b43f7b90066c2e6ffc14"
        "0000000000000000000000000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000b1d332780236"
        "d58a3ab5f5b771b5c40f8cef3fb07c2f1d8d454c58aa4861196d49c11e8d8f1cda04eb"
        "f9c68062254b0af0f60f316b12e34e305ffc047a6864503c01a514ef3a8ea580562ea4"
        "1d6722a950368969cd1955fbfe36d3ce4c74b2a79103c030ed848b6ddeecf36dcb741c"
        "04099fd3df0efdad0550155ad7502fb372a673dae89b025a42d1c7f8dc26ab669f8fcc"
        "62a1c1a8746abf33162b11be1e4de7f00ff0f8291e55107033cc1e011bc666d70e74f0"
        "0a31273335d049da809c31546342dab4190c0661757261203d2a9d1100000000045250"
        "5352909967accd39b9c4ca2c1079e7006681eb62dfc9e5e95d04e851ab85fb7ae415da"
        "cee72f0705617572610101dcc0b61d3ba124b9f39e56e8e5118e1d07b36ccbd2501f4a"
        "ae8807b645510850aea680ba954d8d905bae53c1c863a2ea9dd8e5f98f5699eb3225ef"
        "d38884698501000000080000000000010000002c01ea030000ccacb97d99b529a166cf"
        "35cb03ae3f33fdd9d684e651d1df9c746dc8536fe892000200fcc90000000000000000"
        "000000000000000000000000000000000000006a2d39bf82b9328d6644994c89d3a8c2"
        "f03ef173ff305702a91c1d0aadb9d1688db32080ed7fc8f57be63e5f6d80827827c1c8"
        "3395d3f8dad15e32d39494007ee4849c40cae072eabed8b430533791836d8c3db91a58"
        "05409817b310c5aea7f300000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000000000"
        "000000007308c472433bd6ebfada3e935903589f1d2950ee83fc8fbc3c6d0b8feeb15b"
        "9a7335745c862a2a04a3c3e4ce3bbbd077f47da6fe324f59618f6987d087ac21382b55"
        "325fe3c51bd7a7b15673c9f43af345f40d2792bcb79935a5bda92d8a05fc9103f3d77f"
        "badac1061446fb93d4265b1078cf3fcc59005e615fee741bb634482748fe2875028d0a"
        "ceb6590fe3c73ffad5aacf193221e85816a87d6e78bfd15746d325e58441b37bce6184"
        "129e1cc04cd8ff8c826ad0b35f80feff119f235dabdaf657f2723a0c0661757261203e"
        "2a9d11000000000452505352903d84ec2604bc9c1af7eb380fb54160a601deb8b090c7"
        "7e15e568c4534afca807d2e72f07056175726101017a861b04f2713dd0133d95914d06"
        "eb53603bf1b486e39bfc6a709e316d8cea4ab33fae15f2a0b14e1a84fcd33cbc111bf7"
        "9fc5a7ef830de8a37970ef605eb38b02000000090000000000010000002c01ec030000"
        "d3a927d4b1547a0d854584a84b68e382ff246d85b041f2212b53a4280f1e80ad000300"
        "fcc9000000000000000000000000000000000000000000000000000000bb32d0ca0bd1"
        "35e43a623ab7cb09e0d1f32796ad8c233e4ed8feec648d29bad3f5b6543f107b130b9b"
        "d710652ca118b0b6759672704198f76e32ca3b2e6cec525eceb7bf3c7cd5e1b0392d7e"
        "0e6e8cab04e53c93c0096d5f94c4dd8bab42bb2b000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000000000"
        "00000000000000000000000000007964d3e38f4867e0350fe7a5fb37381cd48fe4b394"
        "f032cf2bcd5270d9a0d4877e1adff5b1995ba375600e8dd09ba8dfe6e0a39fff759f0a"
        "288886c7942d98f72c67ba618710f471d9e967723ded2096cf5f71ebd08e861992e54d"
        "256431275a9103c83496f5cf796e0b6de4210d6e25dc118e79acd0b7e3f1262572bb92"
        "de8f48f9e269e801d1b6b54ae08d7745e17849ec08659fcb5bb994aeb2259e0be73294"
        "9d66f926cf3cd34baa91a2b7486dd91233c06bcb7836f0317c74a07cc5751190275133"
        "3c4c0c0661757261203d2a9d11000000000452505352909967accd39b9c4ca2c1079e7"
        "006681eb62dfc9e5e95d04e851ab85fb7ae415dacee72f0705617572610101e8d70f79"
        "164febc8b9c4dea301fdc1fd34b618b4565e6b026c3713c1402a05389d8f0cbac0242a"
        "301eb46c473e6503fa1d291511a79d51fa7ef72633f0a64080030000000a0000000000"
        "010000002c00e9030000ccacb97d99b529a166cf35cb03ae3f33fdd9d684e651d1df9c"
        "746dc8536fe892000100fcc90000000000000000000000000000000000000000000000"
        "000000001a4c3ccddbfe05cf5334f689f9c2b96ea4efad3a6d424949bf6e9d8226f9dd"
        "e419b60491cd868c49511f44d39eb4040fa16c84e1445f036c05f3ed8148032b74d488"
        "3327a19ba9726fd6eb93e24126b649c0a353ab227c9a7e06b564b8d8d37c0000000000"
        "0000000000000000000000000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000074e3f6a461448d25e8dffe"
        "c76c7cc02afc4b0edc64a1c9eb8e3febaeb658485349c11e8d8f1cda04ebf9c6806225"
        "4b0af0f60f316b12e34e305ffc047a68645031e8897ec9a97b23dc1374b5ce5245dd9e"
        "335dda150e0ba6767964debe1a67b99103b1d332780236d58a3ab5f5b771b5c40f8cef"
        "3fb07c2f1d8d454c58aa4861196ddee89b0261a18b4e07aa7011675649c88cda5d1348"
        "590db6a8cf574db2b13aa568f7955fd5a95eeebd3c29e27550abe0bbe9709a626c36ea"
        "571d568d7c0948377f7655cf0c0661757261203e2a9d11000000000452505352903d84"
        "ec2604bc9c1af7eb380fb54160a601deb8b090c77e15e568c4534afca807d2e72f0705"
        "617572610101f2996e28d6b32d27d50d2dbfd886a70b85dc33739d372d14ff6a03ee6d"
        "4f4f51fa824eaf14c7cdc63c06a3472ccd80faf9d3da0e73698b1d8584c80cb38a8f85"
        "01000000080000000000010000002c00ea030000ccacb97d99b529a166cf35cb03ae3f"
        "33fdd9d684e651d1df9c746dc8536fe892000200fcc900000000000000000000000000"
        "0000000000000000000000000000f071f395a23dcfb8397dfacb114dcfa7c0a0a71c6a"
        "7a29f5ffd6203264fa71386aeb43225bad4fe40c08a2a19501bdbd1e093d5e5445322e"
        "54934c02fed8ee06d12bb73d02efa89aec673f2bb48d8d1eeb6f5a0c62f28b3c4cafc3"
        "46b725f412000000000000000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000000000000000000000c8"
        "21a783d83f85f50a73d145c4c02071ceda0c0b784cff5f58183840adf9b7427335745c"
        "862a2a04a3c3e4ce3bbbd077f47da6fe324f59618f6987d087ac2138b35c9b93d0764e"
        "f466bcfae7b568761f492aafff36275588d66b6c3309f2992191037308c472433bd6eb"
        "fada3e935903589f1d2950ee83fc8fbc3c6d0b8feeb15b9a0229750262e1f267c82a75"
        "ad1408553fffd8facd8d74e370928d79cc4fd877613c53a8b6e42eb65a63c31ef7982d"
        "56f9ec464c0bc0489d5bb547a3f904bce897dd1709d70c0661757261203e2a9d110000"
        "00000452505352903d84ec2604bc9c1af7eb380fb54160a601deb8b090c77e15e568c4"
        "534afca807d2e72f070561757261010116674539add94a3f9da7ffcec0041c1da229af"
        "a22cdc8a46b9bfe1207597db4952b34c2b10be0512dbe7b5903950a63e8c5c38f9f68a"
        "2d7d9dd6096ee476848302000000090000000000010000002c00ec030000ccacb97d99"
        "b529a166cf35cb03ae3f33fdd9d684e651d1df9c746dc8536fe892000300fcc9000000"
        "000000000000000000000000000000000000000000000000e3d000bb6571133eee0442"
        "3e7c02053944e7f432ce56bdb421ba5cd24bd31d9701daced6f0028c5590e10b733a07"
        "4d6eb6e4c848ddd4318429f9a2a1b22828b13da053250d1d14f83e810e5b99e75de0ce"
        "aa209fe3de580ea1063537aac07bbb0000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000271ffe8f3a1207057c63c958cf5fc64bd2d55d83e31327ba668d"
        "39f3091146857e1adff5b1995ba375600e8dd09ba8dfe6e0a39fff759f0a288886c794"
        "2d98f77a9462a89ddda944be0c84b552284bc2dcacb7ab0611158d2d464a2b5437a35f"
        "91037964d3e38f4867e0350fe7a5fb37381cd48fe4b394f032cf2bcd5270d9a0d487e6"
        "69e801384f3658fd3deef68a696a4db4aa55c9cafd05f5d28923c6e38a3bc4a4a74f58"
        "4e3baadfb5f3b96ea7ecfc0903b4f306c7a7a52304393afe187f8e986a446c6a0c0661"
        "757261203e2a9d11000000000452505352903d84ec2604bc9c1af7eb380fb54160a601"
        "deb8b090c77e15e568c4534afca807d2e72f07056175726101017a05e81d233b220839"
        "4447277562e766b98337206ca14100947f640e81c3c717424188d014f191af9973623a"
        "61559b3c37c1e0bc8bd628a464ddf3a54f3f6584030000000a0000000000010000002c"
        "00fa070000df21d43f878b35fb3b79e5af0830a1e83dfdbec45f6a3b9a9567f252fab4"
        "ca82001100fcc9000000000000000000000000000000000000000000000000000000ff"
        "e892375569ee2074668e31c9731d90bcdc2eae8444fa09ac2e27d00c8c30a9eba4bd39"
        "4cd421d32282997f0c144e72cdeebc1b594a8ff2e0fce7be316d8f7877d5145bb600ff"
        "d256a92ff61ade3e8455c2b15f0dc4a486e31a2ccf305c629c00000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000452ef36a236bf43e5d294c6c4de9120b"
        "4eb966f831ff540cd1fd902cdc94427b7301a28882cca10f83db6fdc5c95b81909a7f6"
        "c1ab22318924d733a88b68a0c0777e1d7af2b1ea5d00ad434e55c98a152929fad956e6"
        "1171008feda79ff50dc191034a53e15b5221e57c7bf4265407297ea4f35010710b862c"
        "93b655fcc111e38478f6ce7f0034b40f5ab6c9aeaf8b0b9359e3cb2583035c17e33385"
        "f1013a596748c7c350c2b64d7bad552ec3c38044abfc072cec31ebace457ec7b37a9ca"
        "2b7b1a935b63dd0c0661757261201f95ce0800000000045250535290db2b4e76f1a38a"
        "3aeef6252183453b4b5beb34f417aa3077fbbb7f54e4b0f1aad6e72f07056175726101"
        "013ebff62fd5381146973a02b46aefe8b9574a8fc88dcfeb341618947064d79b452b88"
        "0fc5b5d1d07d89e2179defaeda303d7cb6be14110ef916d50f1b9ae893861100000004"
        "0000000000010000002c00b7090000df21d43f878b35fb3b79e5af0830a1e83dfdbec4"
        "5f6a3b9a9567f252fab4ca82000700fcc9000000000000000000000000000000000000"
        "000000000000000000363986c268944278855d491a4c18684d4b50560e6255c6b65584"
        "6b5ffea4a5806010072de6e3292429e23b247d3322595109046dd8bfef387285f6e6da"
        "5b446d2f5f74f489a5f4e4faa43a1dacd2ed48cd41795ce68305c52e8b15bdf84e99f9"
        "0000000000000000000000000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000000000008ec15f1c7dfa"
        "9e3dc9b6361c4e295e5e5ef04e4b1fd9e2f10d7eab0cfc47786cce76768a4d9db3a9db"
        "ea8acdefeec037a1119dafdc98c62c30c90453ef3a2c755c2b5a2f5c3ad640a03d2b8b"
        "f5c441db0ce24ad5084e712471e45f1a34183aee9103e6250c64c5f9bf518f63185459"
        "b03c6a5b036076eba1da3d4798494b99776ad3d6811300399faae22e642bac4865f9df"
        "a8acb033babd8e9f5f47d8fcb980bce70b4e7d3cce9a004e8ecdfbe906007c38eab54a"
        "72c33e0d4b300e7e12716f2efc41d4fd0f0c0661757261203f2a9d1100000000045250"
        "535290db2b4e76f1a38a3aeef6252183453b4b5beb34f417aa3077fbbb7f54e4b0f1aa"
        "d6e72f0705617572610101da1ca6e728cccb2129807736d8ed86fb09170069c3473b68"
        "2d4b18ae0f7b6038219e8f467b0f0559c31a2d8482f0d20de270cbfd64a43d43ceb6a9"
        "3eb8e2c681070000000e0000000000010000002c00b8090000ccacb97d99b529a166cf"
        "35cb03ae3f33fdd9d684e651d1df9c746dc8536fe892000800fcc90000000000000000"
        "000000000000000000000000000000000000001e46b888dc5336701904ea1ba786a779"
        "d8f24667b48c13aab0557d947dc20bbc885aaebf5e3fe22658c06b277d5af8d88fc200"
        "b0d069f2367a6da8ae0cd380d2661e824bc95eea08db053e2e6baaabba085f36f198e4"
        "cff771741526f70c298300000000000000000000000000000000000000000000000000"
        "0000000000000000000000000000000000000000000000000000000000000000000000"
        "000000002d245c505c11f0c3a9cf96e781b5904f6c1a1e9503a630ccae841901cdcb47"
        "bc9cd302e2215a071302a7b64f00d9cd2e14a6b48e026349599ebaf7bc5fd27515da4f"
        "987356acea7d8bb90d439807ea4e7689d09c281694569275c9aa831bd9d1b903f3760c"
        "1e7536ab5e9451464f83f27a3102851d4acf8a2170db19823a77a76c795e6303005138"
        "7185a4a5904efa33b359ba5a28e072fdc776d1c11c62e491ecd5264be476363727e29e"
        "4646fde75ec292edfa741e2216feb3a2d8a2f00b029e28a44fe5681006434d4c531001"
        "0001040661757261201f95ce08000000000452505352903d84ec2604bc9c1af7eb380f"
        "b54160a601deb8b090c77e15e568c4534afca807d2e72f0705617572610101a237692a"
        "544d0994aa30fca4cb4b401d332d645fe2f0e4b7a673457fb6ce1d789b26ee089270ec"
        "ea375066fb2aebd266d12ff5a13fcd3bbdc76507b0d5f55e8c080000000f0000000000"
        "01000000000007f47290db07c151020000\"}";

    url_loader_factory_.AddResponse(testnet_url, expected_res);

    auto result = future.Take();
    ASSERT_TRUE(result.has_value());

    auto dict = base::test::ParseJsonDict(expected_res);
    const auto* expected_events = dict.FindString("result");
    ASSERT_TRUE(expected_events);

    EXPECT_EQ("0x" + base::HexEncodeLower(result.value()), *expected_events);
  }

  {
    // Invalid block hash was provided.

    url_loader_factory_.ClearResponses();
    url_loader_factory_.AddResponse(testnet_url, R"(
      {"jsonrpc":"2.0",
       "id":1,
       "error":{
         "code":4003,
         "message":"Client error: UnknownBlock: Header was not found in the database: 0x240ecc246eaeea2f75c69fde3d4ce1a7f9761eeaf0a37a9b5e0458f4da9e2812"
       }
      })");

    polkadot_substrate_rpc_->GetEvents(chain_id, block_hash,
                                       future.GetCallback());

    auto result = future.Take();

    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(
        result.error(),
        "Client error: UnknownBlock: Header was not found in the database: "
        "0x240ecc246eaeea2f75c69fde3d4ce1a7f9761eeaf0a37a9b5e0458f4da9e2812");
  }

  {
    // Invalid hex.

    url_loader_factory_.ClearResponses();

    url_loader_factory_.AddResponse(
        testnet_url, R"({"jsonrpc":"2.0","id":1,"result": "cat"})");

    polkadot_substrate_rpc_->GetEvents(chain_id, block_hash,
                                       future.GetCallback());

    auto result = future.Take();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), WalletParsingErrorMessage());
  }

  {
    // No result

    url_loader_factory_.ClearResponses();

    url_loader_factory_.AddResponse(testnet_url, R"({"jsonrpc":"2.0","id":1})");

    polkadot_substrate_rpc_->GetEvents(chain_id, block_hash,
                                       future.GetCallback());

    auto result = future.Take();
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), WalletParsingErrorMessage());
  }
}

}  // namespace brave_wallet
