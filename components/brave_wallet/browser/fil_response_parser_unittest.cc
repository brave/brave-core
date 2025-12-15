/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_response_parser.h"

#include <memory>
#include <string>
#include <utility>

#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;

namespace brave_wallet {

namespace {

base::Value GetResponse(const std::string& value) {
  std::string response = "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":{value}}";
  base::ReplaceSubstringsAfterOffset(&response, 0, "{value}", value);
  return ParseJson(response);
}
}  // namespace

TEST(FilResponseParserUnitTest, ParseFilGetBalance) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"10000000000000000000000000000\"}";
  std::string value;
  EXPECT_EQ(ParseFilGetBalance(ParseJson(json)),
            "10000000000000000000000000000");

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"\"}";
  EXPECT_TRUE(ParseFilGetBalance(ParseJson(json))->empty());
}

TEST(FilResponseParserUnitTest, ParseFilGetTransactionCount) {
  std::string json =
      R"({"jsonrpc":2.0,"id":1,"result":"18446744073709551615"})";

  EXPECT_EQ(ParseFilGetTransactionCount(ParseJson(json)), UINT64_MAX);

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"1\"}";
  EXPECT_EQ(ParseFilGetTransactionCount(ParseJson(json)), 1u);

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":-1}";
  EXPECT_FALSE(ParseFilGetTransactionCount(ParseJson(json)));

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":1.2}";
  EXPECT_FALSE(ParseFilGetTransactionCount(ParseJson(json)));

  EXPECT_FALSE(ParseFilGetTransactionCount(base::Value()));

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":1}";
  EXPECT_FALSE(ParseFilGetTransactionCount(ParseJson(json)));

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{}}";
  EXPECT_FALSE(ParseFilGetTransactionCount(ParseJson(json)));

  json = "{\"jsonrpc\":\"2.0\",\"id\":1}";
  EXPECT_FALSE(ParseFilGetTransactionCount(ParseJson(json)));
}

TEST(FilResponseParserUnitTest, ParseFilEstimateGas) {
  std::string json =
      R"({
          "id": 1,
          "jsonrpc": "2.0",
          "result": {
              "CID": {
                "/": "bafy2bzacebefvj6623fkmfwazpvg7qxgomhicefeb6tunc7wbvd2ee4uppfkw"
              },
              "From": "1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
              "GasFeeCap": "101520",
              "GasLimit": "2187060",
              "GasPremium": "100466",
              "Method": 0,
              "Nonce": 1,
              "Params": "",
              "To": "t1tquwkjo6qvweah2g2yikewr7y5dyjds42pnrn3a",
              "Value": "1000000000000000000",
              "Version": 0
          }
      })";

  auto parse_result = ParseFilEstimateGas(ParseJson(json));
  EXPECT_TRUE(parse_result);
  EXPECT_EQ(parse_result->gas_premium, "100466");
  EXPECT_EQ(parse_result->gas_fee_cap, "101520");
  EXPECT_EQ(parse_result->gas_limit, 2187060u);

  json.clear();
  EXPECT_FALSE(ParseFilEstimateGas(base::Value()));

  json = "[]";
  EXPECT_FALSE(ParseFilEstimateGas(ParseJson(json)));
  // result is not a dictionary
  json = "{\"jsonrpc\":\"2.0\",\"result\":[]}";
  EXPECT_FALSE(ParseFilEstimateGas(ParseJson(json)));

  // No GasLimit
  json = R"({
            "id": 1,
            "jsonrpc": "2.0",
            "result": {
                "CID": {
                  "/": "bafy2bzacebefvj6623fkmfwazpvg7qxgomhicefeb6tunc7wbvd2ee4uppfkw"
                },
                "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
                "GasFeeCap": "101520",
                "GasPremium": "100466",
                "Method": 0,
                "Nonce": 1,
                "Params": "",
                "To": "t1tquwkjo6qvweah2g2yikewr7y5dyjds42pnrn3a",
                "Value": "1000000000000000000",
                "Version": 0
            }
          })";
  EXPECT_FALSE(ParseFilEstimateGas(ParseJson(json)));

  // No GasPremium
  json =
      R"({
          "id": 1,
          "jsonrpc": "2.0",
          "result": {
              "CID": {
                "/": "bafy2bzacebefvj6623fkmfwazpvg7qxgomhicefeb6tunc7wbvd2ee4uppfkw"
              },
              "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
              "GasFeeCap": "101520",
              "GasLimit": "2187060",
              "Method": 0,
              "Nonce": 1,
              "Params": "",
              "To": "t1tquwkjo6qvweah2g2yikewr7y5dyjds42pnrn3a",
              "Value": "1000000000000000000",
              "Version": 0
          }
      })";
  EXPECT_FALSE(ParseFilEstimateGas(ParseJson(json)));

  // No GasFeeCap
  json =
      R"({
          "id": 1,
          "jsonrpc": "2.0",
          "result": {
              "CID": {
                "/": "bafy2bzacebefvj6623fkmfwazpvg7qxgomhicefeb6tunc7wbvd2ee4uppfkw"
              },
              "From": "t1h5tg3bhp5r56uzgjae2373znti6ygq4agkx4hzq",
              "GasLimit": "2187060",
              "GasPremium": "100466",
              "Method": 0,
              "Nonce": 1,
              "Params": "",
              "To": "t1tquwkjo6qvweah2g2yikewr7y5dyjds42pnrn3a",
              "Value": "1000000000000000000",
              "Version": 0
          }
      })";
  EXPECT_FALSE(ParseFilEstimateGas(ParseJson(json)));
}

TEST(FilResponseParserUnitTest, ParseFilGetChainHead) {
  std::optional<uint64_t> height = ParseFilGetChainHead(GetResponse(R"({
        "Blocks":[],
        "Cids": [{
              "/": "bafy2bzaceauxm7waysuftonc4vod6wk4trdjx2ibw233dos6jcvkf5nrhflju"
        }],
        "Height": "18446744073709551615"
      })"));
  EXPECT_TRUE(height);
  EXPECT_EQ(*height, 18446744073709551615u);
  EXPECT_FALSE(ParseFilGetChainHead(base::Value()));
  EXPECT_FALSE(ParseFilGetChainHead(GetResponse(R"({})")));
  EXPECT_FALSE(ParseFilGetChainHead(GetResponse(R"({"Height": 11})")));
  EXPECT_FALSE(ParseFilGetChainHead(GetResponse(R"({"Height": "abc"})")));
  EXPECT_FALSE(ParseFilGetChainHead(GetResponse(R"({"Height": {}})")));
  EXPECT_FALSE(ParseFilGetChainHead(GetResponse(R"({"Height": []})")));
  EXPECT_FALSE(ParseFilGetChainHead(GetResponse(R"({"Height": ""})")));
}

TEST(FilResponseParserUnitTest, ParseFilStateSearchMsgLimited) {
  std::optional<int64_t> exit_code = ParseFilStateSearchMsgLimited(
      GetResponse(
          "{\"Message\": {\"/\":\"cid\"},\"Receipt\": {\"ExitCode\":\"" +
          base::NumberToString(INT64_MAX) + "\"}}"),
      "cid");
  EXPECT_TRUE(exit_code);
  EXPECT_EQ(*exit_code, INT64_MAX);
  exit_code = ParseFilStateSearchMsgLimited(
      GetResponse(
          "{\"Message\": {\"/\":\"cid\"},\"Receipt\": {\"ExitCode\":\"" +
          base::NumberToString(INT64_MIN) + "\"}}"),
      "cid");
  EXPECT_TRUE(exit_code);
  EXPECT_EQ(*exit_code, INT64_MIN);
  EXPECT_FALSE(ParseFilStateSearchMsgLimited(base::Value(), "cid"));
  EXPECT_FALSE(ParseFilStateSearchMsgLimited(ParseJson("{}"), "cid"));
  EXPECT_FALSE(ParseFilStateSearchMsgLimited(
      GetResponse(R"({"Message": {"/":"cid"},"Receipt": {}})"), "cid"));
  EXPECT_FALSE(ParseFilStateSearchMsgLimited(
      GetResponse(R"({"Message": {"/":"cid"},"Receipt": []})"), "cid"));
  EXPECT_FALSE(ParseFilStateSearchMsgLimited(
      GetResponse(R"({"Message": {"/":"cid"},"Receipt": {"ExitCode": "a"}})"),
      "cid"));
  EXPECT_FALSE(ParseFilStateSearchMsgLimited(
      GetResponse(R"({"Message": {"/":"cid"},"Receipt": {"ExitCode": []}})"),
      "cid"));
  EXPECT_FALSE(ParseFilStateSearchMsgLimited(
      GetResponse(R"({"Message": {"/":"cid"},"Receipt": {"ExitCode": {}}})"),
      "cid"));
  EXPECT_FALSE(ParseFilStateSearchMsgLimited(GetResponse("null"), "cid"));
}

TEST(FilResponseParserUnitTest, ParseSendFilecoinTransaction) {
  std::optional<std::string> cid = ParseSendFilecoinTransaction(GetResponse(R"({
    "/": "bafy2bzacea3wsdh6y3a36tb3skempjoxqpuyompjbmfeyf34fi3uy6uue42v4"
  })"));
  EXPECT_TRUE(cid);
  EXPECT_EQ(*cid,
            "bafy2bzacea3wsdh6y3a36tb3skempjoxqpuyompjbmfeyf34fi3uy6uue42v4");
  EXPECT_FALSE(ParseSendFilecoinTransaction(GetResponse(R"({})")));
  EXPECT_FALSE(ParseSendFilecoinTransaction(GetResponse("1")));
  EXPECT_FALSE(ParseSendFilecoinTransaction(GetResponse("[]")));
}

}  // namespace brave_wallet
