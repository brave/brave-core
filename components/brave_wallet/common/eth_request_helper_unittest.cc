/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/common/eth_request_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(EthRequestHelperUnitTest, ParseEthSendTransactionParams) {
  std::string json(
      R"({
        "params": [{
          "from": "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C8",
          "to": "0x7f84E0DfF3ffd0af78770cF86c1b1DdFF99d51C7",
          "gas": "0x146",
          "gasPrice": "0x123",
          "value": "0x25F38E9E0000000",
          "data": "0x010203"
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

  // Invalid things to pass in for parsing
  EXPECT_FALSE(ParseEthSendTransactionParams("not json data", &from));
  EXPECT_FALSE(ParseEthSendTransactionParams("{\"params\":[{},{}]}", &from));
  EXPECT_FALSE(ParseEthSendTransactionParams("{\"params\":[0]}", &from));
  EXPECT_FALSE(ParseEthSendTransactionParams("{}", &from));
  EXPECT_FALSE(ParseEthSendTransactionParams("[]", &from));
  EXPECT_FALSE(ParseEthSendTransactionParams("[[]]", &from));
  EXPECT_FALSE(ParseEthSendTransactionParams("[0]", &from));
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

  // Invalid things to pass in for parsing
  EXPECT_FALSE(ParseEthSendTransaction1559Params("not json data", &from));
  EXPECT_FALSE(ParseEthSendTransactionParams("{\"params\":[{},{}]}", &from));
  EXPECT_FALSE(ParseEthSendTransactionParams("{\"params\":[0]}", &from));
  EXPECT_FALSE(ParseEthSendTransaction1559Params("{}", &from));
  EXPECT_FALSE(ParseEthSendTransaction1559Params("[]", &from));
  EXPECT_FALSE(ParseEthSendTransaction1559Params("[[]]", &from));
  EXPECT_FALSE(ParseEthSendTransaction1559Params("[0]", &from));
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
      "{\"id\":null,\"jsonrpc\":\"2.0\",\"method\":\"eth_blockNumber\","
      "\"params\":[]}";
  EXPECT_TRUE(NormalizeEthRequest(partial_json, &output_json));
  EXPECT_EQ(expected_full_json_no_id, output_json);

  // Invalid input
  EXPECT_FALSE(NormalizeEthRequest(
      "There is only one thing we say to death: Not today.", &output_json));
}

}  // namespace brave_wallet
