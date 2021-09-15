/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/renderer/eth_request_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(EthRequestParserUnitTest, ParseEthSendTransactionParams) {
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

TEST(EthResponseParserUnitTest, ParseEthSendTransaction1559Params) {
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

}  // namespace brave_wallet
