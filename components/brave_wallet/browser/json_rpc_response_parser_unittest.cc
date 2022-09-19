/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "components/grit/brave_components_strings.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

void CompareJSON(const std::string& request_string,
                 const std::string& expected_request) {
  auto request_json = base::JSONReader::Read(request_string);
  ASSERT_TRUE(request_json);
  auto expected_request_json = base::JSONReader::Read(expected_request);
  ASSERT_TRUE(expected_request_json);
  EXPECT_EQ(*request_json, *expected_request_json);
}

}  // namespace

namespace brave_wallet {

TEST(JsonRpcResponseParserUnitTest, ParseSingleStringResult) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"abc\"}";
  std::string value;
  EXPECT_TRUE(brave_wallet::ParseSingleStringResult(json, &value));
  EXPECT_EQ(value, "abc");

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"\"}";
  EXPECT_TRUE(brave_wallet::ParseSingleStringResult(json, &value));
  EXPECT_TRUE(value.empty());
}

TEST(JsonRpcResponseParserUnitTest, ParseDecodedBytesResult) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x556f1830\"}";
  EXPECT_EQ(std::vector<uint8_t>({0x55, 0x6f, 0x18, 0x30}),
            brave_wallet::ParseDecodedBytesResult(json));

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"\"}";
  EXPECT_FALSE(brave_wallet::ParseDecodedBytesResult(json));
}

TEST(JsonRpcResponseParserUnitTest, ParseBoolResult) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x0000000000000000000000000000000000000000000000000000000000000001\"}";
  bool value;
  EXPECT_TRUE(brave_wallet::ParseBoolResult(json, &value));
  EXPECT_TRUE(value);

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x0000000000000000000000000000000000000000000000000000000000000000\"}";
  EXPECT_TRUE(brave_wallet::ParseBoolResult(json, &value));
  EXPECT_FALSE(value);

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x00000000000000000000000000000000000000000\"}";
  EXPECT_FALSE(brave_wallet::ParseBoolResult(json, &value));

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0\"}";
  EXPECT_FALSE(brave_wallet::ParseBoolResult(json, &value));
}

TEST(JsonRpcResponseParserUnitTest, ParseErrorResult) {
  mojom::ProviderError eth_error;
  mojom::SolanaProviderError solana_error;
  std::string eth_error_message;
  std::string solana_error_message;
  std::string json =
      R"({
         "jsonrpc": "2.0",
         "id": 1,
         "error": {
           "code": -32601,
           "message": "method does not exist"
         }
       })";

  // kMethodNotFound = -32601
  ParseErrorResult<mojom::ProviderError>(json, &eth_error, &eth_error_message);
  EXPECT_EQ(eth_error, mojom::ProviderError::kMethodNotFound);
  EXPECT_EQ(eth_error_message, "method does not exist");

  ParseErrorResult<mojom::SolanaProviderError>(json, &solana_error,
                                               &solana_error_message);
  EXPECT_EQ(solana_error, mojom::SolanaProviderError::kMethodNotFound);
  EXPECT_EQ(solana_error_message, "method does not exist");

  // No message should still work
  json =
      R"({
       "jsonrpc": "2.0",
       "id": 1,
       "error": {
         "code": -32601
       }
     })";
  ParseErrorResult<mojom::ProviderError>(json, &eth_error, &eth_error_message);
  EXPECT_EQ(eth_error, mojom::ProviderError::kMethodNotFound);
  EXPECT_TRUE(eth_error_message.empty());

  ParseErrorResult<mojom::SolanaProviderError>(json, &solana_error,
                                               &solana_error_message);
  EXPECT_EQ(solana_error, mojom::SolanaProviderError::kMethodNotFound);
  EXPECT_TRUE(solana_error_message.empty());

  std::vector<std::string> errors{
      R"({
         "jsonrpc": "2.0",
         "id": 1,
         "error": {
           "message": "method does not exist"
         }
       })",
      R"({"jsonrpc": "2.0", "id": 1, "result": "0"})",
      R"({"jsonrpc": "2.0", "id": 1, "error": "0"})",
      R"({"jsonrpc": "2.0", "id": 1, "error": "0"})",
      R"({"jsonrpc": "2.0", "id": 1, "error": {}})",
      "some string",
  };

  for (const std::string& json_error : errors) {
    ParseErrorResult<mojom::ProviderError>(json_error, &eth_error,
                                           &eth_error_message);
    EXPECT_EQ(eth_error, mojom::ProviderError::kParsingError);
    EXPECT_EQ(eth_error_message,
              l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

    ParseErrorResult<mojom::SolanaProviderError>(json_error, &solana_error,
                                                 &solana_error_message);
    EXPECT_EQ(solana_error, mojom::SolanaProviderError::kParsingError);
    EXPECT_EQ(solana_error_message,
              l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

TEST(JsonRpcResponseParserUnitTest, ConvertUint64ToString) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":" + std::to_string(UINT64_MAX) +
      "}";

  EXPECT_EQ(ConvertUint64ToString("/result", json).value(),
            "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"" +
                std::to_string(UINT64_MAX) + "\"}");

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":1}";
  EXPECT_EQ(ConvertUint64ToString("/result", json).value(),
            "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"1\"}");

  EXPECT_FALSE(ConvertUint64ToString("", json));

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":-1}";
  EXPECT_FALSE(ConvertUint64ToString("/result", json));

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":1.2}";
  EXPECT_FALSE(ConvertUint64ToString("/result", json));

  json = "bad json";
  EXPECT_FALSE(ConvertUint64ToString("/result", json));

  EXPECT_FALSE(ConvertUint64ToString("/result", ""));

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"1\"}";
  EXPECT_FALSE(ConvertUint64ToString("/result", json));

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{}}";
  EXPECT_FALSE(ConvertUint64ToString("/result", json));

  json = R"({"jsonrpc":"2.0","id":1,"result":{"value":18446744073709551615}})";
  EXPECT_EQ(
      *ConvertUint64ToString("/result/value", json),
      R"({"id":1,"jsonrpc":"2.0","result":{"value":"18446744073709551615"}})");

  json = R"({"jsonrpc": "2.0", "id": 1,
             "error": {
               "code":-32601,
               "message":"method does not exist"
             }
            })";
  EXPECT_EQ(*ConvertUint64ToString("/result", json), json);
}

TEST(JsonRpcResponseParserUnitTest, ConvertMultiUint64ToString) {
  std::string json =
      R"({"id":1,"jsonrpc":"2.0","result":{
            "value":18446744073709551615,
            "a":{"b":18446744073709551615}
            }})";
  std::string expected_json =
      R"({"id":1,"jsonrpc":"2.0","result":{
            "value":"18446744073709551615",
            "a":{"b":"18446744073709551615"}
            }})";

  CompareJSON(
      *ConvertMultiUint64ToString({"/result/value", "/result/a/b"}, json),
      expected_json);

  EXPECT_FALSE(ConvertMultiUint64ToString({}, json));
  EXPECT_FALSE(ConvertMultiUint64ToString({"", "/result/value"}, json));
  EXPECT_FALSE(ConvertMultiUint64ToString({"result/value", "/result/a/b"}, ""));

  // Fail all if one of the path fails.
  json = R"({"result":{"value":18446744073709551615,"bad":-1}})";
  EXPECT_FALSE(
      ConvertMultiUint64ToString({"/result/value", "/result/bad"}, json));
}

TEST(JsonRpcResponseParserUnitTest, ConvertMultiUint64InObjectArrayToString) {
  std::string json =
      R"({"result":{"array":[
           {"key1":18446744073709551615,"key2":18446744073709551615},
           {"key1":18446744073709551615,"key2":18446744073709551615}
           ]}})";
  std::string expected_json =
      R"({"result":{"array":[
           {"key1":"18446744073709551615","key2":"18446744073709551615"},
           {"key1":"18446744073709551615","key2":"18446744073709551615"}
           ]}})";
  CompareJSON(*ConvertMultiUint64InObjectArrayToString("/result/array",
                                                       {"key1", "key2"}, json),
              expected_json);

  EXPECT_FALSE(
      ConvertMultiUint64InObjectArrayToString("", {"key1", "key2"}, json));
  EXPECT_FALSE(ConvertMultiUint64InObjectArrayToString("/result/array",
                                                       {"key1", ""}, json));
  EXPECT_FALSE(
      ConvertMultiUint64InObjectArrayToString("/result/array", {}, json));
  EXPECT_FALSE(ConvertMultiUint64InObjectArrayToString("/result/array",
                                                       {"key1", "key2"}, ""));

  // Fail all if one of the key fails.
  json = R"({"result":{"array":[
              {"key1":18446744073709551615,"key2":18446744073709551615},
              {"key1":-1,"key2":1}
              ]}})";
  EXPECT_FALSE(ConvertMultiUint64InObjectArrayToString("/result/array",
                                                       {"key1", "key2"}, json));
}

TEST(JsonRpcResponseParserUnitTest, ConvertInt64ToString) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":" + std::to_string(INT64_MAX) +
      "}";

  EXPECT_EQ(ConvertInt64ToString("/result", json).value(),
            "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"" +
                std::to_string(INT64_MAX) + "\"}");

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":" + std::to_string(INT64_MIN) +
      "}";

  EXPECT_EQ(ConvertInt64ToString("/result", json).value(),
            "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"" +
                std::to_string(INT64_MIN) + "\"}");

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":1}";
  EXPECT_EQ(ConvertInt64ToString("/result", json).value(),
            "{\"id\":1,\"jsonrpc\":\"2.0\",\"result\":\"1\"}");

  EXPECT_FALSE(ConvertInt64ToString("", json));

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":1.2}";
  EXPECT_FALSE(ConvertInt64ToString("/result", json));

  json = "bad json";
  EXPECT_FALSE(ConvertInt64ToString("/result", json));

  EXPECT_FALSE(ConvertInt64ToString("/result", ""));

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"1\"}";
  EXPECT_FALSE(ConvertInt64ToString("/result", json));

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":{}}";
  EXPECT_FALSE(ConvertInt64ToString("/result", json));

  json = R"({"jsonrpc": "2.0", "id": 1,
             "error": {
               "code":-32601,
               "message":"method does not exist"
             }
            })";
  EXPECT_EQ(*ConvertInt64ToString("/result", json), json);
}

}  // namespace brave_wallet
