/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>
#include <charconv>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/fil_response_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(FilResponseParserUnitTest, ParseFilGetBalance) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"10000000000000000000000000000\"}";
  std::string value;
  EXPECT_TRUE(brave_wallet::ParseFilGetBalance(json, &value));
  EXPECT_EQ(value, "10000000000000000000000000000");

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"\"}";
  EXPECT_TRUE(brave_wallet::ParseFilGetBalance(json, &value));
  EXPECT_TRUE(value.empty());
}

TEST(FilResponseParserUnitTest, ParseFilGetTransactionCount) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\": 18446744073709551615 }";
  uint64_t value = 0;
  EXPECT_TRUE(brave_wallet::ParseFilGetTransactionCount(json, &value));
  EXPECT_EQ(value, 18446744073709551615u);

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"\"}";
  std::string balance;
  EXPECT_TRUE(brave_wallet::ParseFilGetBalance(json, &balance));
  EXPECT_TRUE(balance.empty());
}

TEST(FilResponseParserUnitTest, ParseFilEstimateGas) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"result\":{\"Version\":0,\"To\":"
      "\"t1tquwkjo6qvweah2g2yikewr7y5dyjds42pnrn3a\",\"From\":"
      "\"t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq\",\"Nonce\":1,\"Value\":"
      "\"1000000000000000000\",\"GasLimit\":2187060,\"GasFeeCap\":\"101520\","
      "\"GasPremium\":\"100466\",\"Method\":0,\"Params\":\"\",\"CID\":{\"/"
      "\":\"bafy2bzacebefvj6623fkmfwazpvg7qxgomhicefeb6tunc7wbvd2ee4uppfkw\"}},"
      "\"id\":1}";
  std::string gas_premium;
  std::string gas_fee_cap;
  uint64_t gas_limit;
  std::string cid;
  EXPECT_TRUE(brave_wallet::ParseFilEstimateGas(
      json, &gas_premium, &gas_fee_cap, &gas_limit, &cid));
  EXPECT_EQ(gas_premium, "100466");
  EXPECT_EQ(gas_fee_cap, "101520");
  EXPECT_EQ(gas_limit, 2187060u);
  EXPECT_EQ(cid,
            "bafy2bzacebefvj6623fkmfwazpvg7qxgomhicefeb6tunc7wbvd2ee4uppfkw");

  gas_premium.clear();
  gas_fee_cap.clear();
  cid.clear();
  gas_limit = 0;
  json.clear();
  EXPECT_FALSE(brave_wallet::ParseFilEstimateGas(
      json, &gas_premium, &gas_fee_cap, &gas_limit, &cid));
  EXPECT_TRUE(gas_premium.empty());
  EXPECT_TRUE(gas_fee_cap.empty());
  EXPECT_TRUE(cid.empty());
  EXPECT_EQ(gas_limit, 0u);
}

}  // namespace brave_wallet
