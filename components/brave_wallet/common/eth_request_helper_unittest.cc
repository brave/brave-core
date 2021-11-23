/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_wallet/common/eth_request_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(EthRequestHelperUnitTest, CommonParseErrors) {
  // Invalid things to pass in for parsing
  const std::vector<std::string> error_cases(
      {"not json data", "{\"params\":[{},{}]}", "{\"params\":[0]}", "{}", "[]",
       "[[]]", "[0]"});
  for (const auto& error_case : error_cases) {
    std::string from;
    EXPECT_FALSE(ParseEthSendTransactionParams(error_case, &from));
    EXPECT_FALSE(ParseEthSendTransaction1559Params(error_case, &from));
    std::string address;
    std::string message;
    EXPECT_FALSE(ParseEthSignParams(error_case, &address, &message));
    std::string chain_id;
    EXPECT_FALSE(ParseSwitchEthereumChainParams(error_case, &chain_id));
  }
}

TEST(EthRequestHelperUnitTest, ParseEthSendTransactionParams) {
  std::string json(
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
      })");
  std::string from;
  mojom::TxDataPtr tx_data = ParseEthSendTransactionParams(json, &from);
  ASSERT_TRUE(tx_data);
  EXPECT_EQ(from, "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C8");
  EXPECT_EQ(tx_data->to, "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C7");
  EXPECT_EQ(tx_data->gas_limit, "0x146");
  EXPECT_EQ(tx_data->gas_price, "0x123");
  EXPECT_EQ(tx_data->value, "0x25F38E9E0000000");
  EXPECT_EQ(tx_data->data, (std::vector<uint8_t>{1, 2, 3}));
  EXPECT_TRUE(tx_data->nonce.empty());  // Should be ignored.
}

TEST(EthResponseHelperUnitTest, ParseEthSendTransaction1559Params) {
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
  mojom::TxData1559Ptr tx_data = ParseEthSendTransaction1559Params(json, &from);
  ASSERT_TRUE(tx_data);
  EXPECT_EQ(from, "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C8");
  EXPECT_EQ(tx_data->base_data->to,
            "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C7");
  EXPECT_EQ(tx_data->base_data->gas_limit, "0x146");
  EXPECT_EQ(tx_data->base_data->gas_price.empty(), true);
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
  tx_data = ParseEthSendTransaction1559Params(json, &from);
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
}

TEST(EthResponseHelperUnitTest, ShouldCreate1559Tx) {
  const std::string ledger_address =
      "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C9";
  const std::string trezor_address =
      "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51CA";

  mojom::AccountInfoPtr primary_account = mojom::AccountInfo::New(
      "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C8", "primary", false, nullptr);
  mojom::AccountInfoPtr ledger_account = mojom::AccountInfo::New(
      ledger_address, "ledger", false,
      mojom::HardwareInfo::New("m/44'/60'/1'/0/0", "Ledger", "123"));
  mojom::AccountInfoPtr trezor_account = mojom::AccountInfo::New(
      trezor_address, "trezor", false,
      mojom::HardwareInfo::New("m/44'/60'/1'/0/0", "Trezor", "123"));
  std::vector<mojom::AccountInfoPtr> account_infos;
  account_infos.push_back(std::move(primary_account));
  account_infos.push_back(std::move(ledger_account));
  account_infos.push_back(std::move(trezor_account));

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
  auto tx_data = ParseEthSendTransaction1559Params(json, &from);

  ASSERT_TRUE(tx_data);
  EXPECT_TRUE(ShouldCreate1559Tx(tx_data.Clone(),
                                 true /* network_supports_eip1559 */,
                                 account_infos, from));
  EXPECT_TRUE(
      ShouldCreate1559Tx(tx_data.Clone(), true, account_infos, ledger_address));
  // From is not found in the account infos, can happen when keyring is locked.
  EXPECT_TRUE(ShouldCreate1559Tx(
      tx_data.Clone(), true /* network_supports_eip1559 */, {}, from));
  // Network don't support EIP1559
  EXPECT_FALSE(ShouldCreate1559Tx(tx_data.Clone(), false, account_infos, from));
  // Keyring don't support EIP1559
  EXPECT_FALSE(
      ShouldCreate1559Tx(tx_data.Clone(), true, account_infos, trezor_address));
  EXPECT_FALSE(ShouldCreate1559Tx(tx_data.Clone(), true, account_infos,
                                  base::ToLowerASCII(trezor_address)));

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

  tx_data = ParseEthSendTransaction1559Params(json, &from);
  ASSERT_TRUE(tx_data);
  EXPECT_TRUE(ShouldCreate1559Tx(tx_data.Clone(),
                                 true /* network_supports_eip1559 */,
                                 account_infos, from));
  EXPECT_FALSE(ShouldCreate1559Tx(tx_data.Clone(), false, account_infos, from));

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
  tx_data = ParseEthSendTransaction1559Params(json, &from);
  ASSERT_TRUE(tx_data);
  EXPECT_FALSE(ShouldCreate1559Tx(tx_data.Clone(),
                                  true /* network_supports_eip1559 */,
                                  account_infos, from));
  EXPECT_FALSE(ShouldCreate1559Tx(tx_data.Clone(), false, account_infos, from));

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
  tx_data = ParseEthSendTransaction1559Params(json, &from);
  ASSERT_TRUE(tx_data);
  EXPECT_TRUE(ShouldCreate1559Tx(tx_data.Clone(), true, account_infos, from));
  EXPECT_TRUE(ShouldCreate1559Tx(tx_data.Clone(), true, account_infos,
                                 base::ToLowerASCII(from)));
  EXPECT_TRUE(
      ShouldCreate1559Tx(tx_data.Clone(), true, account_infos, ledger_address));
  EXPECT_TRUE(ShouldCreate1559Tx(tx_data.Clone(), true, account_infos,
                                 base::ToLowerASCII(ledger_address)));
  // From is not found in the account infos, can happen when keyring is locked.
  EXPECT_TRUE(ShouldCreate1559Tx(
      tx_data.Clone(), true /* network_supports_eip1559 */, {}, from));

  EXPECT_FALSE(ShouldCreate1559Tx(tx_data.Clone(), false, account_infos, from));
  EXPECT_FALSE(ShouldCreate1559Tx(tx_data.Clone(), false, account_infos, from));
  // Keyring don't support EIP1559
  EXPECT_FALSE(
      ShouldCreate1559Tx(tx_data.Clone(), true, account_infos, trezor_address));
  EXPECT_FALSE(ShouldCreate1559Tx(tx_data.Clone(), true, account_infos,
                                  base::ToLowerASCII(trezor_address)));
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
  std::string address;
  std::string message;
  EXPECT_TRUE(ParseEthSignParams(json, &address, &message));
  EXPECT_EQ(address, "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");
  EXPECT_EQ(message, "0xdeadbeaf");

  EXPECT_FALSE(ParseEthSignParams(json, &address, nullptr));
  EXPECT_FALSE(ParseEthSignParams(json, nullptr, &message));
  EXPECT_FALSE(ParseEthSignParams(json, nullptr, nullptr));
  EXPECT_FALSE(ParseEthSignParams(json_extra_entry, &address, &message));
  EXPECT_FALSE(ParseEthSignParams("{\"params\":[{}]}", &address, &message));
  EXPECT_FALSE(
      ParseEthSignParams("{\"params\":[\"123\",123]}", &address, &message));
}

TEST(EthResponseHelperUnitTest, ParsePersonalSignParams) {
  const std::string json(
      R"({
        "params": [
          "0xdeadbeef",
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
  std::string address;
  std::string message;
  EXPECT_TRUE(ParsePersonalSignParams(json, &address, &message));
  EXPECT_EQ(address, "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");
  EXPECT_EQ(message, "0xdeadbeef");

  EXPECT_TRUE(ParsePersonalSignParams(json_extra_entry, &address, &message));
  EXPECT_EQ(address, "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");
  EXPECT_EQ(message, "0xdeadbeef");

  EXPECT_TRUE(
      ParsePersonalSignParams(json_with_message_string, &address, &message));
  EXPECT_EQ(address, "0x9b2055d370f73ec7d8a03e965129118dc8f5bf83");
  EXPECT_EQ(message, "0x4861766520796f752074726965642042726176653f");

  EXPECT_FALSE(ParsePersonalSignParams(json, &address, nullptr));
  EXPECT_FALSE(ParsePersonalSignParams(json, nullptr, &message));
  EXPECT_FALSE(ParsePersonalSignParams(json, nullptr, nullptr));
  EXPECT_FALSE(
      ParsePersonalSignParams("{\"params\":[{}]}", &address, &message));
  EXPECT_FALSE(
      ParseEthSignParams("{\"params\":[\"123\",123]}", &address, &message));
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
  std::string method, params;
  EXPECT_TRUE(GetEthJsonRequestInfo(json, &id, &method, &params));
  EXPECT_EQ(id, base::Value("1"));
  EXPECT_EQ(method, "eth_blockNumber");
  EXPECT_EQ(params, "[]");

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
  EXPECT_EQ(params, "[\"0x5BaD55\",true]");

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
  EXPECT_EQ(params, "[\"0x5BaD55\",true]");

  // Can pass nullptr for id
  method.clear();
  params.clear();
  EXPECT_TRUE(GetEthJsonRequestInfo(json, nullptr, &method, &params));
  EXPECT_EQ(method, "eth_getBlockByNumber");
  EXPECT_EQ(params, "[\"0x5BaD55\",true]");

  // Can pass nullptr for method
  id = base::Value();
  params.clear();
  EXPECT_TRUE(GetEthJsonRequestInfo(json, &id, nullptr, &params));
  EXPECT_EQ(id, base::Value(2));
  EXPECT_EQ(params, "[\"0x5BaD55\",true]");

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
  EXPECT_EQ(params, "[\"0x5BaD55\",true]");

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
  std::string chain_id;
  EXPECT_TRUE(ParseSwitchEthereumChainParams(
      "{\"params\": [{\"chainId\": \"0x1\"}]}", &chain_id));
  EXPECT_EQ(chain_id, "0x1");
  // trailing comma should be accepted
  EXPECT_TRUE(ParseSwitchEthereumChainParams(
      "{\"params\": [{\"chainId\": \"0x1\",},]}", &chain_id));
  EXPECT_EQ(chain_id, "0x1");

  EXPECT_FALSE(ParseSwitchEthereumChainParams(
      "{\"params\": [{\"chainId\": 0x1}]}", &chain_id));
  EXPECT_FALSE(ParseSwitchEthereumChainParams(
      "{\"params\": [{\"chainId\": \"123\"}]}", &chain_id));
  EXPECT_FALSE(ParseSwitchEthereumChainParams(
      "{\"params\": [{\"chainId\": [123]}]}", &chain_id));
  EXPECT_FALSE(ParseSwitchEthereumChainParams(
      "{\"params\": [{\"chain_idea\": \"0x1\"}]}", &chain_id));
  EXPECT_FALSE(ParseSwitchEthereumChainParams(
      "{\"params\": [{\"chain_id\": \"0x1\"}]}", &chain_id));
  EXPECT_FALSE(ParseSwitchEthereumChainParams("{\"params\": [{}]}", &chain_id));
}

TEST(EthRequestHelperUnitTest, ParseEthSignTypedDataParams) {
  const std::string json = R"({
    "params": [
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
        \"message\": {
          \"from\": {
            \"name\":\"Cow\", \"wallet\":\"0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826\"
          },
          \"to\": {
            \"name\":\"Bob\", \"wallet\":\"0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbB\"
          },
          \"contents\":\"Hello, Bob!\"
        }
      }"
    ]
  })";

  std::string address;
  std::string message;
  base::Value domain;
  std::vector<uint8_t> message_to_sign;

  EXPECT_TRUE(ParseEthSignTypedDataParams(
      json, &address, &message, &message_to_sign, &domain,
      EthSignTypedDataHelper::Version::kV4));

  EXPECT_EQ(address, "0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826");
  EXPECT_EQ(
      message,
      "{\"contents\":\"Hello, "
      "Bob!\",\"from\":{\"name\":\"Cow\",\"wallet\":"
      "\"0xCD2a3d9F938E13CD947Ec05AbC7FE734Df8DD826\"},\"to\":{\"name\":"
      "\"Bob\",\"wallet\":\"0xbBbBBBBbbBBBbbbBbbBbbbbBBbBbbbbBbBbbBBbB\"}}");

  std::string* ds_name = domain.FindStringKey("name");
  ASSERT_TRUE(ds_name);
  EXPECT_EQ(*ds_name, "Ether Mail");
  std::string* ds_version = domain.FindStringKey("version");
  ASSERT_TRUE(ds_version);
  EXPECT_EQ(*ds_version, "1");
  auto chain_id = domain.FindIntKey("chainId");
  ASSERT_TRUE(chain_id);
  EXPECT_EQ(*chain_id, 1);
  std::string* ds_verifying_contract =
      domain.FindStringKey("verifyingContract");
  ASSERT_TRUE(ds_verifying_contract);
  EXPECT_EQ(*ds_verifying_contract,
            "0xCcCCccccCCCCcCCCCCCcCcCccCcCCCcCcccccccC");

  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(message_to_sign)),
            "be609aee343fb3c4b28e1df9e632fca64fcfaede20f02e86244efddf30957bd2");
}

}  // namespace brave_wallet
