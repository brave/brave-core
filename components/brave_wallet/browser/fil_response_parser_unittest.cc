/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/strcat.h"
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

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"\"}";
  EXPECT_TRUE(brave_wallet::ParseFilGetBalance(json, &value));
  EXPECT_TRUE(value.empty());
}

TEST(FilResponseParserUnitTest, ParseFilGetTransactionCount) {
  auto max_nonce = UINT64_MAX;
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":" + std::to_string(max_nonce) +
      "}";
  uint64_t value = 0;
  EXPECT_TRUE(brave_wallet::ParseFilGetTransactionCount(json, &value));
  EXPECT_EQ(value, max_nonce);

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":1}";
  value = 0;
  EXPECT_TRUE(brave_wallet::ParseFilGetTransactionCount(json, &value));
  EXPECT_EQ(value, 1u);

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":-1}";
  EXPECT_FALSE(brave_wallet::ParseFilGetTransactionCount(json, &value));

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":1.2}";
  EXPECT_FALSE(brave_wallet::ParseFilGetTransactionCount(json, &value));

  json = "bad json";
  EXPECT_FALSE(brave_wallet::ParseFilGetTransactionCount(json, &value));

  EXPECT_FALSE(brave_wallet::ParseFilGetTransactionCount("", &value));

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"1\"}";
  EXPECT_FALSE(brave_wallet::ParseFilGetTransactionCount(json, &value));

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{}}";
  EXPECT_FALSE(brave_wallet::ParseFilGetTransactionCount(json, &value));

  json = "{\"jsonrpc\":\"2.0\",\"id\":1}";
  EXPECT_FALSE(brave_wallet::ParseFilGetTransactionCount(json, &value));
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
  int64_t gas_limit = 0;
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

  json = "broken";
  EXPECT_FALSE(brave_wallet::ParseFilEstimateGas(
      json, &gas_premium, &gas_fee_cap, &gas_limit, &cid));
  // result is not a dictionary
  json = "{\"jsonrpc\":\"2.0\",\"result\":[]}";
  EXPECT_FALSE(brave_wallet::ParseFilEstimateGas(
      json, &gas_premium, &gas_fee_cap, &gas_limit, &cid));

  // No GasLimit
  json =
      "{\"jsonrpc\":\"2.0\",\"result\":{\"Version\":0,\"To\":"
      "\"t1tquwkjo6qvweah2g2yikewr7y5dyjds42pnrn3a\",\"From\":"
      "\"t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq\",\"Nonce\":1,\"Value\":"
      "\"1000000000000000000\",\"GasFeeCap\":\"101520\","
      "\"GasPremium\":\"100466\",\"Method\":0,\"Params\":\"\",\"CID\":{\"/"
      "\":\"bafy2bzacebefvj6623fkmfwazpvg7qxgomhicefeb6tunc7wbvd2ee4uppfkw\"}},"
      "\"id\":1}";
  EXPECT_FALSE(brave_wallet::ParseFilEstimateGas(
      json, &gas_premium, &gas_fee_cap, &gas_limit, &cid));

  // No GasPremium
  json =
      "{\"jsonrpc\":\"2.0\",\"result\":{\"Version\":0,\"To\":"
      "\"t1tquwkjo6qvweah2g2yikewr7y5dyjds42pnrn3a\",\"From\":"
      "\"t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq\",\"Nonce\":1,\"Value\":"
      "\"1000000000000000000\",\"GasLimit\":2187060,\"GasFeeCap\":\"101520\","
      "\"Method\":0,\"Params\":\"\",\"CID\":{\"/"
      "\":\"bafy2bzacebefvj6623fkmfwazpvg7qxgomhicefeb6tunc7wbvd2ee4uppfkw\"}},"
      "\"id\":1}";
  EXPECT_FALSE(brave_wallet::ParseFilEstimateGas(
      json, &gas_premium, &gas_fee_cap, &gas_limit, &cid));

  // No GasFeeCap
  json =
      "{\"jsonrpc\":\"2.0\",\"result\":{\"Version\":0,\"To\":"
      "\"t1tquwkjo6qvweah2g2yikewr7y5dyjds42pnrn3a\",\"From\":"
      "\"t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq\",\"Nonce\":1,\"Value\":"
      "\"1000000000000000000\",\"GasLimit\":2187060,"
      "\"GasPremium\":\"100466\",\"Method\":0,\"Params\":\"\",\"CID\":{\"/"
      "\":\"bafy2bzacebefvj6623fkmfwazpvg7qxgomhicefeb6tunc7wbvd2ee4uppfkw\"}},"
      "\"id\":1}";
  EXPECT_FALSE(brave_wallet::ParseFilEstimateGas(
      json, &gas_premium, &gas_fee_cap, &gas_limit, &cid));

  // No CID
  json =
      "{\"jsonrpc\":\"2.0\",\"result\":{\"Version\":0,\"To\":"
      "\"t1tquwkjo6qvweah2g2yikewr7y5dyjds42pnrn3a\",\"From\":"
      "\"t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq\",\"Nonce\":1,\"Value\":"
      "\"1000000000000000000\",\"GasLimit\":2187060,\"GasFeeCap\":\"101520\","
      "\"GasPremium\":\"100466\",\"Method\":0,\"Params\":\"\"},"
      "\"id\":1}";

  EXPECT_FALSE(brave_wallet::ParseFilEstimateGas(
      json, &gas_premium, &gas_fee_cap, &gas_limit, &cid));

  // Empty CID
  json =
      "{\"jsonrpc\":\"2.0\",\"result\":{\"Version\":0,\"To\":"
      "\"t1tquwkjo6qvweah2g2yikewr7y5dyjds42pnrn3a\",\"From\":"
      "\"t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq\",\"Nonce\":1,\"Value\":"
      "\"1000000000000000000\",\"GasLimit\":2187060,\"GasFeeCap\":\"101520\","
      "\"GasPremium\":\"100466\",\"Method\":0,\"Params\":\"\",\"CID\":{},"
      "\"id\":1}";
  EXPECT_FALSE(brave_wallet::ParseFilEstimateGas(
      json, &gas_premium, &gas_fee_cap, &gas_limit, &cid));

  // Broken CID
  json =
      "{\"jsonrpc\":\"2.0\",\"result\":{\"Version\":0,\"To\":"
      "\"t1tquwkjo6qvweah2g2yikewr7y5dyjds42pnrn3a\",\"From\":"
      "\"t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq\",\"Nonce\":1,\"Value\":"
      "\"1000000000000000000\",\"GasLimit\":2187060,\"GasFeeCap\":\"101520\","
      "\"GasPremium\":\"100466\",\"Method\":0,\"Params\":\"\",\"CID\":[],"
      "\"id\":1}";
  EXPECT_FALSE(brave_wallet::ParseFilEstimateGas(
      json, &gas_premium, &gas_fee_cap, &gas_limit, &cid));
}

TEST(FilResponseParserUnitTest, ParseFilGetChainHead) {
  std::string cid;
  EXPECT_TRUE(brave_wallet::ParseFilGetChainHead(GetResponse(R"({
        "Blocks":[],
        "Cids": [{
              "/": "bafy2bzaceauxm7waysuftonc4vod6wk4trdjx2ibw233dos6jcvkf5nrhflju"
        }],
        "Height": 22452
      })"),
                                                 &cid));
  EXPECT_EQ(cid,
            "bafy2bzaceauxm7waysuftonc4vod6wk4trdjx2ibw233dos6jcvkf5nrhflju");
  EXPECT_FALSE(brave_wallet::ParseFilGetChainHead(GetResponse(R"()"), &cid));
  EXPECT_FALSE(brave_wallet::ParseFilGetChainHead(GetResponse(R"({})"), &cid));
  EXPECT_FALSE(brave_wallet::ParseFilGetChainHead(GetResponse(R"({,})"), &cid));
  EXPECT_FALSE(
      brave_wallet::ParseFilGetChainHead(GetResponse(R"({"Cids": 11})"), &cid));
  EXPECT_FALSE(brave_wallet::ParseFilGetChainHead(
      GetResponse(R"({"Cids": "abc"})"), &cid));
  EXPECT_FALSE(
      brave_wallet::ParseFilGetChainHead(GetResponse(R"({"Cids": {}})"), &cid));
  EXPECT_FALSE(
      brave_wallet::ParseFilGetChainHead(GetResponse(R"({"Cids": []})"), &cid));
  EXPECT_FALSE(brave_wallet::ParseFilGetChainHead(
      GetResponse(R"({"Cids": [{}]})"), &cid));
  EXPECT_FALSE(
      brave_wallet::ParseFilGetChainHead(R"({"Cids": [{"/":1}]})", &cid));
  EXPECT_FALSE(
      brave_wallet::ParseFilGetChainHead(R"({"Cids": [{"/":[]}]})", &cid));
  EXPECT_FALSE(
      brave_wallet::ParseFilGetChainHead(R"({"Cids": [{"/":{}}]})", &cid));
}

}  // namespace brave_wallet
