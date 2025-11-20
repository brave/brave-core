/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/eth_request_helper.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

using base::test::ParseJsonDict;
using base::test::ParseJsonList;
using ::testing::Optional;

namespace brave_wallet {
namespace {
base::Value::List ParseParamsList(const std::string& json) {
  return std::move(*ParseJsonDict(json).FindList("params"));
}
}  // namespace

TEST(EthRequestHelperUnitTest, CommonParseErrors) {
  // Invalid things to pass in for parsing
  auto list_error_cases =
      std::to_array({"[{},{}]", "[0]", "[]", "[[]]", "[0]"});
  for (const auto& error_case : list_error_cases) {
    auto list_params = base::test::ParseJsonList(error_case);

    std::string from;
    EXPECT_FALSE(ParseEthTransaction1559Params(list_params, from));
    std::string address;
    std::string message;
    EXPECT_FALSE(ParseEthSignParams(list_params));
    std::string chain_id;
    EXPECT_FALSE(ParseSwitchEthereumChainParams(list_params));
    EXPECT_FALSE(ParseWalletWatchAssetParams(list_params, message));
    std::string signature;
    EXPECT_FALSE(ParsePersonalEcRecoverParams(list_params));
    EXPECT_FALSE(ParseEthGetEncryptionPublicKeyParams(list_params));
    EXPECT_FALSE(ParseEthDecryptParams(list_params));
  }

  auto dict_error_cases =
      std::to_array({"{\"params\":[{},{}]}", "{\"params\":[0]}", "{}"});
  for (const auto& error_case : dict_error_cases) {
    auto dict_params = base::test::ParseJsonDict(error_case);

    EXPECT_FALSE(ParseEthDecryptData(dict_params));
  }
}

TEST(EthResponseHelperUnitTest, ParseEthTransaction1559Params) {
  std::string json(
      R"({
        "params": [{
          "from": "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C8",
          "to": "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C7",
          "gas": "0x146",
          "value": "0x25F38E9E0000000",
          "data": "0x010203",
          "nonce": "0x01",
          "maxPriorityFeePerGas": "0x1",
          "maxFeePerGas": "0x2"
        }]
      })");
  std::string from;
  mojom::TxData1559Ptr tx_data =
      ParseEthTransaction1559Params(ParseParamsList(json), from);
  ASSERT_TRUE(tx_data);
  EXPECT_EQ(from, "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C8");
  EXPECT_EQ(tx_data->base_data->to,
            "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C7");
  EXPECT_EQ(tx_data->base_data->gas_limit, "0x146");
  EXPECT_TRUE(tx_data->base_data->gas_price.empty());
  EXPECT_EQ(tx_data->base_data->value, "0x25F38E9E0000000");
  EXPECT_EQ(tx_data->base_data->data, (std::vector<uint8_t>{1, 2, 3}));
  EXPECT_EQ(tx_data->max_priority_fee_per_gas, "0x1");
  EXPECT_EQ(tx_data->max_fee_per_gas, "0x2");
  EXPECT_TRUE(tx_data->base_data->nonce.empty());  // Should be ignored.

  json =
      R"({
        "params": [{
          "from": "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C8",
          "to": "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C7",
          "gas": "0x146",
          "value": "0x25F38E9E0000000",
          "data": "0x010203"
        }]
      })";
  tx_data = ParseEthTransaction1559Params(ParseParamsList(json), from);
  ASSERT_TRUE(tx_data);
  EXPECT_EQ(from, "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C8");
  EXPECT_EQ(tx_data->base_data->to,
            "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C7");
  EXPECT_EQ(tx_data->base_data->gas_limit, "0x146");
  EXPECT_EQ(tx_data->base_data->gas_price.empty(), true);
  EXPECT_EQ(tx_data->base_data->value, "0x25F38E9E0000000");
  EXPECT_EQ(tx_data->base_data->data, (std::vector<uint8_t>{1, 2, 3}));
  // Allowed to parse without these fields, the client should determine
  // reasonable values in this case.
  EXPECT_TRUE(tx_data->max_priority_fee_per_gas.empty());
  EXPECT_TRUE(tx_data->max_fee_per_gas.empty());

  // deploying contract
  json =
      R"({
        "params": [{
          "from": "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C8",
          "gas": "0x146",
          "data": "0x010203"
        }]
      })";
  tx_data = ParseEthTransaction1559Params(ParseParamsList(json), from);
  ASSERT_TRUE(tx_data);
  EXPECT_EQ(from, "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C8");
  EXPECT_TRUE(tx_data->base_data->to.empty());
  EXPECT_EQ(tx_data->base_data->gas_limit, "0x146");
  EXPECT_TRUE(tx_data->base_data->gas_price.empty());
  EXPECT_TRUE(tx_data->base_data->value.empty());
  EXPECT_EQ(tx_data->base_data->data, (std::vector<uint8_t>{1, 2, 3}));
  EXPECT_TRUE(tx_data->max_priority_fee_per_gas.empty());
  EXPECT_TRUE(tx_data->max_fee_per_gas.empty());
}

TEST(EthResponseHelperUnitTest, ShouldCreate1559Tx) {
  // Test both EIP1559 and legacy gas fee fields are specified.
  std::string json(
      R"({
        "params": [{
          "from": "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C8",
          "to": "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C7",
          "gas": "0x146",
          "gasPrice": "0x123",
          "value": "0x25F38E9E0000000",
          "data": "0x010203",
          "nonce": "0x01",
          "maxPriorityFeePerGas": "0x1",
          "maxFeePerGas": "0x2"
        }]
      })");
  std::string from;
  auto tx_data = ParseEthTransaction1559Params(ParseParamsList(json), from);

  ASSERT_TRUE(tx_data);
  EXPECT_TRUE(ShouldCreate1559Tx(*tx_data));

  // Test only EIP1559 gas fee fields are specified.
  json =
      R"({
        "params": [{
          "from": "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C8",
          "to": "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C7",
          "gas": "0x146",
          "value": "0x25F38E9E0000000",
          "data": "0x010203",
          "nonce": "0x01",
          "maxPriorityFeePerGas": "0x1",
          "maxFeePerGas": "0x2"
        }]
      })";

  tx_data = ParseEthTransaction1559Params(ParseParamsList(json), from);
  ASSERT_TRUE(tx_data);
  EXPECT_TRUE(ShouldCreate1559Tx(*tx_data));

  // Test only legacy gas field is specified.
  json =
      R"({
        "params": [{
          "from": "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C8",
          "to": "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C7",
          "gas": "0x146",
          "gasPrice": "0x123",
          "value": "0x25F38E9E0000000",
          "data": "0x010203",
          "nonce": "0x01"
        }]
      })";
  tx_data = ParseEthTransaction1559Params(ParseParamsList(json), from);
  ASSERT_TRUE(tx_data);
  EXPECT_FALSE(ShouldCreate1559Tx(*tx_data));

  // Test no gas fee fields are specified.
  json =
      R"({
        "params": [{
          "from": "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C8",
          "to": "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C7",
          "gas": "0x146",
          "value": "0x25F38E9E0000000",
          "data": "0x010203"
        }]
      })";
  tx_data = ParseEthTransaction1559Params(ParseParamsList(json), from);
  ASSERT_TRUE(tx_data);
  EXPECT_TRUE(ShouldCreate1559Tx(*tx_data));
}

TEST(EthResponseHelperUnitTest, ParseEthSignParams) {
  const std::string json(
      R"({
        "params": [
          "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83",
          "0xdeadbeaf"
        ]
      })");
  const std::string json_extra_entry(
      R"({
        "params": [
          "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83",
          "0xdeadbeaf",
          "0xdeafbeaf"
        ]
      })");
  auto sign_params = ParseEthSignParams(ParseParamsList(json));
  EXPECT_TRUE(sign_params);
  EXPECT_EQ(sign_params->address, "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");
  EXPECT_EQ(sign_params->message, "0xdeadbeaf");

  EXPECT_FALSE(ParseEthSignParams(ParseParamsList(json_extra_entry)));
  EXPECT_FALSE(ParseEthSignParams(ParseParamsList("{\"params\":[{}]}")));
  EXPECT_FALSE(
      ParseEthSignParams(ParseParamsList("{\"params\":[\"123\",123]}")));
}

TEST(EthResponseHelperUnitTest, ParsePersonalSignParams) {
  const std::string json(
      R"({
        "params": [
          "0xdeadbeef",
          "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83"
        ]
      })");
  const std::string json_missing_0x_prefix(
      R"({
        "params": [
          "deadbeef",
          "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83"
        ]
      })");
  const std::string json_extra_entry(
      R"({
        "params": [
          "0xdeadbeef",
          "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83",
          "12345",
          null,
          123
        ]
      })");
  const std::string json_with_message_string(
      R"({
        "params": [
          "Have you tried Brave?",
          "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83"
        ]
      })");
  const std::string json_empty_message(
      R"({
        "params": [
          "",
          "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83"
        ]
      })");
  const std::string json_0x_message(
      R"({
        "params": [
          "0x",
          "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83"
        ]
      })");
  const std::string wrong_order(
      R"({
        "params": [
          "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83",
          "0xdeadbeef"
        ]
      })");
  const std::string two_address_looking_params(
      R"({
        "params": [
          "0x9b2055d370f73ec7d8a03e965129118dc8f5bf84",
          "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83"
        ]
      })");

  auto sign_params = ParsePersonalSignParams(ParseParamsList(json));
  EXPECT_TRUE(sign_params);
  EXPECT_EQ(sign_params->address, "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");
  EXPECT_EQ(sign_params->message, "0xdeadbeef");

  sign_params =
      ParsePersonalSignParams(ParseParamsList(json_missing_0x_prefix));
  EXPECT_TRUE(sign_params);
  EXPECT_EQ(sign_params->address, "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");
  EXPECT_EQ(sign_params->message, "0xdeadbeef");

  sign_params = ParsePersonalSignParams(ParseParamsList(json_extra_entry));
  EXPECT_TRUE(sign_params);
  EXPECT_EQ(sign_params->address, "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");
  EXPECT_EQ(sign_params->message, "0xdeadbeef");

  sign_params =
      ParsePersonalSignParams(ParseParamsList(json_with_message_string));
  EXPECT_TRUE(sign_params);
  EXPECT_EQ(sign_params->address, "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");
  EXPECT_EQ(sign_params->message,
            "0x4861766520796f752074726965642042726176653f");

  sign_params = ParsePersonalSignParams(ParseParamsList(json_empty_message));
  EXPECT_TRUE(sign_params);
  EXPECT_EQ(sign_params->address, "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");
  EXPECT_EQ(sign_params->message, "0x");

  sign_params = ParsePersonalSignParams(ParseParamsList(json_0x_message));
  EXPECT_TRUE(sign_params);
  EXPECT_EQ(sign_params->address, "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");
  // To be consistent with MM :S
  EXPECT_EQ(ToHex("0x"), "0x3078");
  EXPECT_EQ(sign_params->message, "0x3078");

  // MM allows the wrong order and figures out the correct order if the first
  // order is an address by mistake.
  sign_params = ParsePersonalSignParams(ParseParamsList(wrong_order));
  EXPECT_TRUE(sign_params);
  EXPECT_EQ(sign_params->address, "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");
  EXPECT_EQ(sign_params->message, "0xdeadbeef");

  // Make sure that 2 arguments of the same length doesn't re-order
  sign_params =
      ParsePersonalSignParams(ParseParamsList(two_address_looking_params));
  EXPECT_TRUE(sign_params);
  EXPECT_EQ(sign_params->address, "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");
  EXPECT_EQ(sign_params->message, "0x9b2055d370f73ec7d8a03e965129118dc8f5bf84");
  EXPECT_FALSE(ParsePersonalSignParams(ParseParamsList("{\"params\":[{}]}")));
  EXPECT_FALSE(
      ParsePersonalSignParams(ParseParamsList("{\"params\":[\"123\",123]}")));
}

TEST(EthResponseHelperUnitTest, ParsePersonalEcRecoverParams) {
  const std::string json(
      R"({
        "params": [
          "0x68656c6c6f20776f726c64",
          "0xeb0c4e96c69a98dbdd61ac6871e39c12c90e9fa4420a017a23c67f4cc4fd06f43c32ade58cd19ed438ce7e2d7360b59020489e9ac05e56e8637d3e516165c3f11c"
        ]
      })");
  const std::string json_extra_entry(
      R"({
        "params": [
          "0x68656c6c6f20776f726c64",
          "0xeb0c4e96c69a98dbdd61ac6871e39c12c90e9fa4420a017a23c67f4cc4fd06f43c32ade58cd19ed438ce7e2d7360b59020489e9ac05e56e8637d3e516165c3f11c",
          "12345",
          null,
          123
        ]
      })");
  const std::string json_with_message_string(
      R"({
        "params": [
          "hello world",
          "0xeb0c4e96c69a98dbdd61ac6871e39c12c90e9fa4420a017a23c67f4cc4fd06f43c32ade58cd19ed438ce7e2d7360b59020489e9ac05e56e8637d3e516165c3f11c"
        ]
      })");
  auto recover_params = ParsePersonalEcRecoverParams(ParseParamsList(json));
  EXPECT_TRUE(recover_params);
  EXPECT_EQ(recover_params->message, "0x68656c6c6f20776f726c64");
  EXPECT_EQ(
      recover_params->signature,
      "0xeb0c4e96c69a98dbdd61ac6871e39c12c90e9fa4420a017a23c67f4cc4fd06f43c32ad"
      "e58cd19ed438ce7e2d7360b59020489e9ac05e56e8637d3e516165c3f11c");

  recover_params =
      ParsePersonalEcRecoverParams(ParseParamsList(json_extra_entry));
  EXPECT_TRUE(recover_params);
  EXPECT_EQ(recover_params->message, "0x68656c6c6f20776f726c64");
  EXPECT_EQ(
      recover_params->signature,
      "0xeb0c4e96c69a98dbdd61ac6871e39c12c90e9fa4420a017a23c67f4cc4fd06f43c32ad"
      "e58cd19ed438ce7e2d7360b59020489e9ac05e56e8637d3e516165c3f11c");

  recover_params =
      ParsePersonalEcRecoverParams(ParseParamsList(json_with_message_string));
  EXPECT_TRUE(recover_params);
  EXPECT_EQ(recover_params->message, "0x68656c6c6f20776f726c64");
  EXPECT_EQ(
      recover_params->signature,
      "0xeb0c4e96c69a98dbdd61ac6871e39c12c90e9fa4420a017a23c67f4cc4fd06f43c32ad"
      "e58cd19ed438ce7e2d7360b59020489e9ac05e56e8637d3e516165c3f11c");
}

TEST(EthResponseHelperUnitTest, ParseEthGetEncryptionPublicKeyParams) {
  const std::string json(
      R"({
        "params": [
          "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83"
        ]
      })");
  const std::string json_extra_entry(
      R"({
        "params": [
          "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83",
          "12345",
          null,
          123
        ]
      })");

  EXPECT_EQ(ParseEthGetEncryptionPublicKeyParams(ParseParamsList(json)),
            "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");
  EXPECT_EQ(
      ParseEthGetEncryptionPublicKeyParams(ParseParamsList(json_extra_entry)),
      "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");
}

TEST(EthResponseHelperUnitTest, ParseEthDecryptParams) {
  const std::string json(
      R"({
        "params": [
          "0x7b2276657273696f6e223a227832353531392d7873616c736132302d706f6c7931333035222c226e6f6e6365223a224f702f7353626241455474506d704c42337a4933686430693969486e62682f38222c22657068656d5075626c69634b6579223a22474e62315a4d635436335235687962483034563344537551677137674d4d5937705a61354235546b66446f3d222c2263697068657274657874223a224b454a72325564686d4237663838514b7a6a413151666a3650586932784f34357a3377766d673d3d227d",
          "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83"
        ]
      })");
  const std::string json_extra_entry(
      R"({
        "params": [
          "0x7b2276657273696f6e223a227832353531392d7873616c736132302d706f6c7931333035222c226e6f6e6365223a224f702f7353626241455474506d704c42337a4933686430693969486e62682f38222c22657068656d5075626c69634b6579223a22474e62315a4d635436335235687962483034563344537551677137674d4d5937705a61354235546b66446f3d222c2263697068657274657874223a224b454a72325564686d4237663838514b7a6a413151666a3650586932784f34357a3377766d673d3d227d",
          "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83",
          null,
          123
        ]
      })");
  const std::string json_extra_first_entry(
      R"({
        "params": [
          "extra_front",
          "0x7b2276657273696f6e223a227832353531392d7873616c736132302d706f6c7931333035222c226e6f6e6365223a224f702f7353626241455474506d704c42337a4933686430693969486e62682f38222c22657068656d5075626c69634b6579223a22474e62315a4d635436335235687962483034563344537551677137674d4d5937705a61354235546b66446f3d222c2263697068657274657874223a224b454a72325564686d4237663838514b7a6a413151666a3650586932784f34357a3377766d673d3d227d",
          "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83",
        ]
      })");

  auto decrypt_params = ParseEthDecryptParams(ParseParamsList(json));
  EXPECT_TRUE(decrypt_params);
  EXPECT_EQ(
      decrypt_params->untrusted_encrypted_data_json,
      R"({"version":"x25519-xsalsa20-poly1305","nonce":"Op/sSbbAETtPmpLB3zI3hd0i9iHnbh/8","ephemPublicKey":"GNb1ZMcT63R5hybH04V3DSuQgq7gMMY7pZa5B5TkfDo=","ciphertext":"KEJr2UdhmB7f88QKzjA1Qfj6PXi2xO45z3wvmg=="})");
  EXPECT_EQ(decrypt_params->address,
            "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");

  decrypt_params = ParseEthDecryptParams(ParseParamsList(json_extra_entry));
  EXPECT_TRUE(decrypt_params);
  EXPECT_EQ(
      decrypt_params->untrusted_encrypted_data_json,
      R"({"version":"x25519-xsalsa20-poly1305","nonce":"Op/sSbbAETtPmpLB3zI3hd0i9iHnbh/8","ephemPublicKey":"GNb1ZMcT63R5hybH04V3DSuQgq7gMMY7pZa5B5TkfDo=","ciphertext":"KEJr2UdhmB7f88QKzjA1Qfj6PXi2xO45z3wvmg=="})");
  EXPECT_EQ(decrypt_params->address,
            "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");
  EXPECT_FALSE(ParseEthDecryptParams(ParseParamsList(json_extra_first_entry)));
}

TEST(EthResponseHelperUnitTest, ParseEthDecryptData) {
  const std::string json(
      R"({"version":"x25519-xsalsa20-poly1305","nonce":"Op/sSbbAETtPmpLB3zI3hd0i9iHnbh/8","ephemPublicKey":"GNb1ZMcT63R5hybH04V3DSuQgq7gMMY7pZa5B5TkfDo=","ciphertext":"KEJr2UdhmB7f88QKzjA1Qfj6PXi2xO45z3wvmg=="})");

  auto eth_decrypt_data = ParseEthDecryptData(base::test::ParseJsonDict(json));
  ASSERT_TRUE(eth_decrypt_data);
  EXPECT_EQ(eth_decrypt_data->version, "x25519-xsalsa20-poly1305");
  EXPECT_EQ(base::Base64Encode(eth_decrypt_data->nonce),
            "Op/sSbbAETtPmpLB3zI3hd0i9iHnbh/8");
  EXPECT_EQ(base::Base64Encode(eth_decrypt_data->ephemeral_public_key),
            "GNb1ZMcT63R5hybH04V3DSuQgq7gMMY7pZa5B5TkfDo=");
  EXPECT_EQ(base::Base64Encode(eth_decrypt_data->ciphertext),
            "KEJr2UdhmB7f88QKzjA1Qfj6PXi2xO45z3wvmg==");
}

TEST(EthResponseHelperUnitTest, GetEthJsonRequestInfo) {
  // Happy path
  std::string json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "eth_blockNumber",
    "params": []
  })";
  base::Value id;
  std::string method;
  base::Value::List params;
  EXPECT_TRUE(GetEthJsonRequestInfo(json, &id, &method, &params));
  EXPECT_EQ(id, base::Value("1"));
  EXPECT_EQ(method, "eth_blockNumber");
  EXPECT_EQ(params, ParseJsonList("[]"));

  json = R"({
    "id": null,
    "jsonrpc": "2.0",
    "method": "eth_getBlockByNumber",
    "params": ["0x5BaD55",true]
  })";
  method.clear();
  params.clear();
  EXPECT_TRUE(GetEthJsonRequestInfo(json, &id, &method, &params));
  EXPECT_EQ(id, base::Value());
  EXPECT_EQ(method, "eth_getBlockByNumber");
  EXPECT_EQ(params, ParseJsonList("[\"0x5BaD55\",true]"));

  json = R"({
    "id": 2,
    "jsonrpc": "2.0",
    "method": "eth_getBlockByNumber",
    "params": ["0x5BaD55",true]
  })";
  id = base::Value();
  method.clear();
  params.clear();
  EXPECT_TRUE(GetEthJsonRequestInfo(json, &id, &method, &params));
  EXPECT_EQ(id, base::Value(2));
  EXPECT_EQ(method, "eth_getBlockByNumber");
  EXPECT_EQ(params, ParseJsonList("[\"0x5BaD55\",true]"));

  // Can pass nullptr for id
  method.clear();
  params.clear();
  EXPECT_TRUE(GetEthJsonRequestInfo(json, nullptr, &method, &params));
  EXPECT_EQ(method, "eth_getBlockByNumber");
  EXPECT_EQ(params, ParseJsonList("[\"0x5BaD55\",true]"));

  // Can pass nullptr for method
  id = base::Value();
  params.clear();
  EXPECT_TRUE(GetEthJsonRequestInfo(json, &id, nullptr, &params));
  EXPECT_EQ(id, base::Value(2));
  EXPECT_EQ(params, ParseJsonList("[\"0x5BaD55\",true]"));

  // Can pass nullptr for params
  id = base::Value();
  method.clear();
  EXPECT_TRUE(GetEthJsonRequestInfo(json, &id, &method, nullptr));
  EXPECT_EQ(id, base::Value(2));
  EXPECT_EQ(method, "eth_getBlockByNumber");

  // Can pass nullptr for all params
  EXPECT_TRUE(GetEthJsonRequestInfo(json, nullptr, nullptr, nullptr));

  // Can omit id but ask for id return
  std::string missing_id_json = R"({
    "method": "eth_getBlockByNumber",
    "params": ["0x5BaD55",true]
  })";
  id = base::Value("something");
  method.clear();
  params.clear();
  EXPECT_TRUE(GetEthJsonRequestInfo(missing_id_json, &id, &method, &params));
  EXPECT_EQ(id, base::Value());
  EXPECT_EQ(method, "eth_getBlockByNumber");
  EXPECT_EQ(params, ParseJsonList("[\"0x5BaD55\",true]"));

  // Missing method
  std::string missing_method_json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "params": []
  })";
  EXPECT_FALSE(
      GetEthJsonRequestInfo(missing_method_json, &id, &method, &params));

  // Invalid method type
  std::string wrong_type_method_json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": 1,
    "params": []
  })";
  EXPECT_FALSE(
      GetEthJsonRequestInfo(wrong_type_method_json, &id, &method, &params));

  // Invalid params type
  std::string wrong_type_params_json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "eth_getBlockByNumber",
    "params": 1
  })";
  EXPECT_FALSE(
      GetEthJsonRequestInfo(wrong_type_params_json, &id, &method, &params));

  // Not even JSON
  std::string invalid_input = "Your sound card works perfectly!";
  EXPECT_FALSE(GetEthJsonRequestInfo(invalid_input, &id, &method, &params));
}

TEST(EthResponseHelperUnitTest, NormalizeEthRequest) {
  // Identity works
  std::string full_json =
      "{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\",\"params\":"
      "[]}";
  std::string output_json;
  EXPECT_TRUE(NormalizeEthRequest(full_json, &output_json));
  EXPECT_EQ(full_json, output_json);

  // Fills missing id and jsonrpc values
  std::string partial_json = "{\"method\":\"eth_blockNumber\",\"params\":[]}";
  std::string expected_full_json_no_id =
      "{\"id\":\"1\",\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\","
      "\"params\":[]}";
  EXPECT_TRUE(NormalizeEthRequest(partial_json, &output_json));
  EXPECT_EQ(expected_full_json_no_id, output_json);

  // Invalid input
  EXPECT_FALSE(NormalizeEthRequest(
      "There is only one thing we say to death: Not today.", &output_json));
}

TEST(EthResponseHelperUnitTest, ParseSwitchEthereumChainParams) {
  EXPECT_EQ(ParseSwitchEthereumChainParams(
                ParseParamsList("{\"params\": [{\"chainId\": \"0x1\"}]}")),
            "0x1");

  EXPECT_FALSE(ParseSwitchEthereumChainParams(
      ParseParamsList("{\"params\": [{\"chainId\": 1}]}")));
  EXPECT_FALSE(ParseSwitchEthereumChainParams(
      ParseParamsList("{\"params\": [{\"chainId\": \"123\"}]}")));
  EXPECT_FALSE(ParseSwitchEthereumChainParams(
      ParseParamsList("{\"params\": [{\"chainId\": [123]}]}")));
  EXPECT_FALSE(ParseSwitchEthereumChainParams(
      ParseParamsList("{\"params\": [{\"chain_idea\": \"0x1\"}]}")));
  EXPECT_FALSE(ParseSwitchEthereumChainParams(
      ParseParamsList("{\"params\": [{\"chain_id\": \"0x1\"}]}")));
  EXPECT_FALSE(
      ParseSwitchEthereumChainParams(ParseParamsList("{\"params\": [{}]}")));
}

TEST(EthRequestHelperUnitTest, ParseEthSignTypedDataParams) {
  constexpr char kJson[] = R"([
      "0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826",
      "{
        \"types\" :{
          \"EIP712Domain\": [
            { \"name\": \"name\", \"type\": \"string\" },
            { \"name\": \"version\", \"type\": \"string\" },
            { \"name\": \"chainId\", \"type\": \"uint256\" },
            { \"name\": \"verifyingContract\", \"type\": \"address\" },
          ],
          \"Mail\": [
            {\"name\": \"from\", \"type\": \"Person\"},
            {\"name\": \"to\", \"type\": \"Person\"},
            {\"name\": \"contents\", \"type\": \"string\"}
          ],
          \"Person\": [
            {\"name\": \"name\", \"type\": \"string\"},
            {\"name\": \"wallet\", \"type\": \"address\"}
          ]
        },
        \"primaryType\": \"Mail\",
        \"domain\": {
          \"name\": \"Ether Mail\",
          \"version\": \"1\",
          \"chainId\": 1,
          \"verifyingContract\": \"0xCcCCccccCCCCcCCCCCCcCcCccCcCCCcCcccccccC\",
        },
        \"message\": %s
      }"
    ])";

  std::string json = absl::StrFormat(kJson, R"({
    \"from\": {
      \"name\":\"Cow\",
      \"wallet\":\"0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826\"
    },
    \"to\": {
      \"name\":\"Bob\",
      \"wallet\":\"0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbB\"
    },
    \"contents\":\"Hello, Bob!\"
  })");

  const auto& expected_domain =
      R"({
        "name": "Ether Mail",
        "version": "1",
        "chainId": 1,
        "verifyingContract": "0xCcCCccccCCCCcCCCCCCcCcCccCcCCCcCcccccccC",
      })";

  const auto& expected_message =
      "{\"contents\":\"Hello, "
      "Bob!\",\"from\":{\"name\":\"Cow\",\"wallet\":"
      "\"0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826\"},\"to\":{\"name\":"
      "\"Bob\",\"wallet\":\"0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbB\"}}";
  const auto& expected_message_to_sign =
      "be609aee343fb3c4b28e1df9e632fca64fcfaede20f02e86244efddf30957bd2";
  const auto& expected_primary_hash =
      "c52c0ee5d84264471806290a3f2c4cecfc5490626bf912d01f240d7a274b371e";
  const auto& expected_domain_hash =
      "f2cee375fa42b42143804025fc449deafd50cc031ca257e0b194a650a912090f";

  auto params_list = ParseJsonList(json);
  EXPECT_TRUE(params_list[1].is_string());
  auto eth_sign_typed_data = ParseEthSignTypedDataParams(
      params_list, EthSignTypedDataHelper::Version::kV4);

  ASSERT_TRUE(eth_sign_typed_data);

  EXPECT_EQ(eth_sign_typed_data->address_param,
            "0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826");
  EXPECT_EQ(eth_sign_typed_data->message_json, expected_message);

  EXPECT_EQ(ParseJsonDict(eth_sign_typed_data->domain_json),
            ParseJsonDict(expected_domain));

  EXPECT_EQ(eth_sign_typed_data->chain_id, "0x1");

  EXPECT_EQ(base::HexEncodeLower(eth_sign_typed_data->domain_hash),
            expected_domain_hash);
  EXPECT_EQ(base::HexEncodeLower(eth_sign_typed_data->primary_hash),
            expected_primary_hash);
  auto message_to_sign = EthSignTypedDataHelper::GetTypedDataMessageToSign(
      eth_sign_typed_data->domain_hash, eth_sign_typed_data->primary_hash);
  EXPECT_EQ(base::HexEncodeLower(message_to_sign), expected_message_to_sign);
  EXPECT_FALSE(eth_sign_typed_data->meta);

  // Test that TypedData can also be a Dict, instead of a string
  auto str = params_list[1].GetString();
  auto dict =
      base::JSONReader::ReadDict(str, base::JSON_PARSE_CHROMIUM_EXTENSIONS |
                                          base::JSON_ALLOW_TRAILING_COMMAS);
  ASSERT_TRUE(dict);
  params_list[1] = base::Value(dict->Clone());
  EXPECT_TRUE(params_list[1].is_dict());
  eth_sign_typed_data = ParseEthSignTypedDataParams(
      params_list, EthSignTypedDataHelper::Version::kV4);

  ASSERT_TRUE(eth_sign_typed_data);

  EXPECT_EQ(eth_sign_typed_data->address_param,
            "0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826");
  EXPECT_EQ(eth_sign_typed_data->message_json, expected_message);

  EXPECT_EQ(ParseJsonDict(eth_sign_typed_data->domain_json),
            ParseJsonDict(expected_domain));

  EXPECT_EQ(eth_sign_typed_data->chain_id, "0x1");

  EXPECT_EQ(base::HexEncodeLower(eth_sign_typed_data->domain_hash),
            expected_domain_hash);
  EXPECT_EQ(base::HexEncodeLower(eth_sign_typed_data->primary_hash),
            expected_primary_hash);
  message_to_sign = EthSignTypedDataHelper::GetTypedDataMessageToSign(
      eth_sign_typed_data->domain_hash, eth_sign_typed_data->primary_hash);
  EXPECT_EQ(base::HexEncodeLower(message_to_sign), expected_message_to_sign);
  EXPECT_FALSE(eth_sign_typed_data->meta);

  // Test with extra fields in the message.
  json = absl::StrFormat(kJson, R"({
    \"from\": {
      \"name\":\"Cow\",
      \"wallet\":\"0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826\"
    },
    \"to\": {
      \"name\":\"Bob\",
      \"wallet\":\"0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbB\"
    },
    \"contents\":\"Hello, Bob!\",
    \"foo\":\"bar\"
  })");

  params_list = ParseJsonList(json);
  eth_sign_typed_data = ParseEthSignTypedDataParams(
      params_list, EthSignTypedDataHelper::Version::kV4);
  ASSERT_TRUE(eth_sign_typed_data);
  // OK: extraneous message properties are sanitized.
  EXPECT_EQ(eth_sign_typed_data->message_json, expected_message);

  // OK: primary type message hash is unchanged.
  EXPECT_EQ(base::HexEncodeLower(eth_sign_typed_data->primary_hash),
            expected_primary_hash);

  // OK: domain hash is unchanged.
  EXPECT_EQ(base::HexEncodeLower(eth_sign_typed_data->domain_hash),
            expected_domain_hash);

  // OK: message bytes to sign are unchanged.
  message_to_sign = EthSignTypedDataHelper::GetTypedDataMessageToSign(
      eth_sign_typed_data->domain_hash, eth_sign_typed_data->primary_hash);
  EXPECT_EQ(base::HexEncodeLower(message_to_sign), expected_message_to_sign);
  EXPECT_FALSE(eth_sign_typed_data->meta);
}

TEST(EthRequestHelperUnitTest, ParseWalletWatchAssetParams) {
  std::string json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "wallet_watchAsset",
    "params": [{
      "options": {
        "address": "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
        "symbol": "BAT",
        "decimals": 18,
        "image": "https://test.com/test.png"
      },
      "type": "ERC20"
      }]
  })";

  mojom::BlockchainTokenPtr expected_token = mojom::BlockchainToken::New(
      "0x0D8775F648430679A709E98d2b0Cb6250d2887EF", "BAT",
      "https://test.com/test.png", false, true, false, false,
      mojom::SPLTokenProgram::kUnsupported, false, false, "BAT", 18, true, "",
      "", "0x1", mojom::CoinType::ETH, false);

  mojom::BlockchainTokenPtr token;
  std::string error_message;
  token = ParseWalletWatchAssetParams(ParseParamsList(json), error_message);
  EXPECT_TRUE(token);
  token->chain_id = "0x1";
  EXPECT_EQ(token, expected_token);
  EXPECT_TRUE(error_message.empty());

  expected_token->logo = "";

  // Test optional image and non-checksum address.
  json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "wallet_watchAsset",
    "params": [{
      "options": {
        "address": "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
        "symbol": "BAT",
        "decimals": 18,
      },
      "type": "ERC20"
    }]
  })";
  token = ParseWalletWatchAssetParams(ParseParamsList(json), error_message);
  EXPECT_TRUE(token);
  token->chain_id = "0x1";
  EXPECT_EQ(token, expected_token);
  EXPECT_TRUE(error_message.empty());

  // Decimals as string is allowed for web compatibility.
  json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "wallet_watchAsset",
    "params": [{
      "options": {
        "address": "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
        "symbol": "BAT",
        "decimals": "18",
      },
      "type": "ERC20"
    }]
  })";
  token = ParseWalletWatchAssetParams(ParseParamsList(json), error_message);
  EXPECT_TRUE(token);
  token->chain_id = "0x1";
  EXPECT_EQ(token, expected_token);
  EXPECT_TRUE(error_message.empty());

  // Missing type
  json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "wallet_watchAsset",
    "params": [{
      "options": {
        "address": "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
        "symbol": "BAT",
        "decimals": 18,
      }
    }]
  })";
  EXPECT_FALSE(
      ParseWalletWatchAssetParams(ParseParamsList(json), error_message));
  EXPECT_FALSE(error_message.empty());

  // Missing address
  json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "wallet_watchAsset",
    "params": [{
      "options": {
        "symbol": "BAT",
        "decimals": 18,
      },
      "type": "ERC20"
    }]
  })";
  EXPECT_FALSE(
      ParseWalletWatchAssetParams(ParseParamsList(json), error_message));
  EXPECT_FALSE(error_message.empty());

  // Invalid address
  json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "wallet_watchAsset",
    "params": [{
      "options": {
        "address": "123",
        "symbol": "BAT",
        "decimals": 18,
      },
      "type": "ERC20"
    }]
  })";
  EXPECT_FALSE(
      ParseWalletWatchAssetParams(ParseParamsList(json), error_message));
  EXPECT_FALSE(error_message.empty());

  // Missing symbol
  json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "wallet_watchAsset",
    "params": [{
      "options": {
        "address": "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
        "decimals": 18,
      },
      "type": "ERC20"
    }]
  })";
  EXPECT_FALSE(
      ParseWalletWatchAssetParams(ParseParamsList(json), error_message));
  EXPECT_FALSE(error_message.empty());

  // Invalid symbol, len = 12
  json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "wallet_watchAsset",
    "params": [{
      "options": {
        "address": "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
        "symbol": "LOOOOOOOOONG",
        "decimals": 18,
      },
      "type": "ERC20"
    }]
  })";
  EXPECT_FALSE(
      ParseWalletWatchAssetParams(ParseParamsList(json), error_message));
  EXPECT_FALSE(error_message.empty());

  // Missing decimals
  json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "wallet_watchAsset",
    "params": [{
      "options": {
        "address": "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
        "symbol": "BAT",
      },
      "type": "ERC20"
    }]
  })";
  EXPECT_FALSE(
      ParseWalletWatchAssetParams(ParseParamsList(json), error_message));
  EXPECT_FALSE(error_message.empty());

  // Invalid decimals, negative number or larger than 36.
  json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "wallet_watchAsset",
    "params": [{
      "options": {
        "address": "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
        "symbol": "BAT",
        "decimals": -1,
      },
      "type": "ERC20"
    }]
  })";
  EXPECT_FALSE(
      ParseWalletWatchAssetParams(ParseParamsList(json), error_message));
  EXPECT_FALSE(error_message.empty());

  json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "wallet_watchAsset",
    "params": [{
      "options": {
        "address": "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
        "symbol": "BAT",
        "decimals": 37,
      },
      "type": "ERC20"
    }]
  })";
  EXPECT_FALSE(
      ParseWalletWatchAssetParams(ParseParamsList(json), error_message));
  EXPECT_FALSE(error_message.empty());

  // Params in an array should work for legacy send.
  json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "wallet_watchAsset",
    "params": [{
      "options": {
        "address": "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
        "symbol": "BAT",
        "decimals": 18
      },
      "type": "ERC20"
    }]
  })";

  token = ParseWalletWatchAssetParams(ParseParamsList(json), error_message);
  EXPECT_TRUE(token);
  token->chain_id = "0x1";
  EXPECT_EQ(token, expected_token);
  EXPECT_TRUE(error_message.empty());

  // Test image parameter
  json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "wallet_watchAsset",
    "params": [{
      "options": {
        "address": "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
        "symbol": "BAT",
        "decimals": 18,
        "image": "http://test.com/test.png"
      },
      "type": "ERC20"
    }]
  })";
  expected_token->logo = "http://test.com/test.png";
  token = ParseWalletWatchAssetParams(ParseParamsList(json), error_message);
  EXPECT_TRUE(token);
  token->chain_id = "0x1";
  EXPECT_EQ(token, expected_token);
  EXPECT_TRUE(error_message.empty());

  json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "wallet_watchAsset",
    "params": [{
      "options": {
        "address": "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
        "symbol": "BAT",
        "decimals": 18,
        "image": "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd1PeAAAADElEQVR42mP4z8AAAAMBAQD3A0FDAAAAAElFTkSuQmCC"
      },
      "type": "ERC20"
    }]
  })";
  expected_token->logo =
      "data:image/"
      "png;base64,"
      "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAIAAACQd1PeAAAADElEQVR42mP4z8AAAAMBAQD3"
      "A0FDAAAAAElFTkSuQmCC";
  token = ParseWalletWatchAssetParams(ParseParamsList(json), error_message);
  EXPECT_TRUE(token);
  token->chain_id = "0x1";
  EXPECT_EQ(token, expected_token);
  EXPECT_TRUE(error_message.empty());

  // Invalid image parameter will have empty logo string.
  json = R"({
    "id": "1",
    "jsonrpc": "2.0",
    "method": "wallet_watchAsset",
    "params": [{
      "options": {
        "address": "0x0d8775f648430679a709e98d2b0cb6250d2887ef",
        "symbol": "BAT",
        "decimals": 18,
        "image": "test.png"
      },
      "type": "ERC20"
    }]
  })";
  expected_token->logo = "";
  token = ParseWalletWatchAssetParams(ParseParamsList(json), error_message);
  EXPECT_TRUE(token);
  token->chain_id = "0x1";
  EXPECT_EQ(token, expected_token);
  EXPECT_TRUE(error_message.empty());
}

TEST(EthResponseHelperUnitTest, ParseRequestPermissionsParams) {
  std::string json =
      R"({
        "method": "wallet_requestPermissions",
        "params": [{
          "eth_accounts": {},
        }]
      })";
  EXPECT_THAT(ParseRequestPermissionsParams(ParseParamsList(json)),
              Optional(base::flat_set<std::string>{"eth_accounts"}));

  json =
      R"({
        "method": "wallet_requestPermissions",
        "params": [{
          "eth_accounts": {},
          "eth_someFutureMethod": {}
        }]
      })";
  EXPECT_THAT(ParseRequestPermissionsParams(ParseParamsList(json)),
              Optional(base::flat_set<std::string>{"eth_accounts",
                                                   "eth_someFutureMethod"}));

  json =
      R"({
        "method": "wallet_requestPermissions",
        "params": [{}]
      })";
  EXPECT_THAT(ParseRequestPermissionsParams(ParseParamsList(json)),
              Optional(base::flat_set<std::string>()));

  EXPECT_FALSE(ParseRequestPermissionsParams(base::Value::List()));
  EXPECT_FALSE(
      ParseRequestPermissionsParams(ParseParamsList(R"({ "params": [5] })")));
  EXPECT_FALSE(
      ParseRequestPermissionsParams(ParseParamsList(R"({ "params": [] })")));
}

TEST(EthResponseHelperUnitTest, ParseEthSendRawTransaction) {
  constexpr char kExpectedRawTransaction[] =
      "0xf86c0c8525f38e9e0082520894cb08bd29e330594182a05a062441ccdb348aae658801"
      "6345785d8a0000802ea0b9534f8e424fd28eecb16cd771b577df6a933ff58ac8c41786f0"
      "2dcba0b632c1a039d0379cdbfb54cfd1894de3bb5c0583a11bc79abf19b6961e948e2127"
      "f47bec";

  std::string json = absl::StrFormat(
      R"({
        "method": "eth_sendRawTransaction",
        "params": ["%s"]
      })",
      kExpectedRawTransaction);
  EXPECT_EQ(ParseEthSendRawTransactionParams(ParseParamsList(json)),
            kExpectedRawTransaction);
  EXPECT_FALSE(
      ParseEthSendRawTransactionParams(ParseParamsList(R"({"params": []})")));
  EXPECT_FALSE(ParseEthSendRawTransactionParams(
      ParseParamsList(R"({"params": [123]})")));
}

}  // namespace brave_wallet
