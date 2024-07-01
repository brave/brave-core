/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/json_rpc_responses.h"
#include "components/grit/brave_components_strings.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

using base::test::ParseJson;

namespace brave_wallet {

TEST(JsonRpcResponseParserUnitTest, ParseSingleStringResult) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"abc\"}";
  EXPECT_EQ(ParseSingleStringResult(ParseJson(json)), "abc");

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"\"}";
  EXPECT_TRUE(ParseSingleStringResult(ParseJson(json))->empty());
}

TEST(JsonRpcResponseParserUnitTest, ParseDecodedBytesResult) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x556f1830\"}";
  EXPECT_EQ(std::vector<uint8_t>({0x55, 0x6f, 0x18, 0x30}),
            ParseDecodedBytesResult(ParseJson(json)));

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"\"}";
  EXPECT_FALSE(ParseDecodedBytesResult(ParseJson(json)));
}

TEST(JsonRpcResponseParserUnitTest, ParseBoolResult) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x0000000000000000000000000000000000000000000000000000000000000001\"}";
  EXPECT_EQ(ParseBoolResult(ParseJson(json)), std::make_optional(true));

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x0000000000000000000000000000000000000000000000000000000000000000\"}";
  EXPECT_EQ(ParseBoolResult(ParseJson(json)), std::make_optional(false));

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x00000000000000000000000000000000000000000\"}";
  EXPECT_EQ(ParseBoolResult(ParseJson(json)), std::nullopt);

  json = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"0\"}";
  EXPECT_EQ(ParseBoolResult(ParseJson(json)), std::nullopt);
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
  ParseErrorResult<mojom::ProviderError>(ParseJson(json), &eth_error,
                                         &eth_error_message);
  EXPECT_EQ(eth_error, mojom::ProviderError::kMethodNotFound);
  EXPECT_EQ(eth_error_message, "method does not exist");

  ParseErrorResult<mojom::SolanaProviderError>(ParseJson(json), &solana_error,
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
  ParseErrorResult<mojom::ProviderError>(ParseJson(json), &eth_error,
                                         &eth_error_message);
  EXPECT_EQ(eth_error, mojom::ProviderError::kMethodNotFound);
  EXPECT_TRUE(eth_error_message.empty());

  ParseErrorResult<mojom::SolanaProviderError>(ParseJson(json), &solana_error,
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
      R"("some string")",
  };

  for (const std::string& json_error : errors) {
    ParseErrorResult<mojom::ProviderError>(ParseJson(json_error), &eth_error,
                                           &eth_error_message);
    EXPECT_EQ(eth_error, mojom::ProviderError::kParsingError);
    EXPECT_EQ(eth_error_message,
              l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

    ParseErrorResult<mojom::SolanaProviderError>(
        ParseJson(json_error), &solana_error, &solana_error_message);
    EXPECT_EQ(solana_error, mojom::SolanaProviderError::kParsingError);
    EXPECT_EQ(solana_error_message,
              l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }

  // Unknown error code.
  json =
      R"({
       "jsonrpc": "2.0",
       "id": 1,
       "error": {
         "code": 3
       }
     })";
  ParseErrorResult<mojom::ProviderError>(ParseJson(json), &eth_error,
                                         &eth_error_message);
  EXPECT_EQ(mojom::ProviderError::kUnknown, eth_error);
  ParseErrorResult<mojom::SolanaProviderError>(ParseJson(json), &solana_error,
                                               &solana_error_message);
  EXPECT_EQ(mojom::SolanaProviderError::kUnknown, solana_error);
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

  EXPECT_EQ(ParseJson(*ConvertMultiUint64ToString(
                {"/result/value", "/result/a/b"}, json)),
            ParseJson(expected_json));

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
  EXPECT_EQ(ParseJson(*ConvertMultiUint64InObjectArrayToString(
                "/result/array", "", {"key1", "key2"}, json)),
            ParseJson(expected_json));

  EXPECT_FALSE(
      ConvertMultiUint64InObjectArrayToString("", "", {"key1", "key2"}, json));
  EXPECT_FALSE(ConvertMultiUint64InObjectArrayToString("/result/array", "",
                                                       {"key1", ""}, json));
  EXPECT_FALSE(
      ConvertMultiUint64InObjectArrayToString("/result/array", "", {}, json));
  EXPECT_FALSE(ConvertMultiUint64InObjectArrayToString("/result/array", "",
                                                       {"key1", "key2"}, ""));

  // Fail all if one of the key fails.
  json = R"({"result":{"array":[
              {"key1":18446744073709551615,"key2":18446744073709551615},
              {"key1":-1,"key2":1}
              ]}})";
  EXPECT_FALSE(ConvertMultiUint64InObjectArrayToString("/result/array", "",
                                                       {"key1", "key2"}, json));

  // Works for nested keys
  json =
      R"({"result":{"array":[
           {"sub_path":{"key1":18446744073709551615,"key2":18446744073709551615}},
           {"sub_path":{"key1":18446744073709551615,"key2":18446744073709551615}}
           ]}})";
  expected_json =
      R"({"result":{"array":[
           {"sub_path": {"key1":"18446744073709551615","key2":"18446744073709551615"}},
           {"sub_path": {"key1":"18446744073709551615","key2":"18446744073709551615"}}
           ]}})";
  EXPECT_EQ(ParseJson(*ConvertMultiUint64InObjectArrayToString(
                "/result/array", "/sub_path", {"key1", "key2"}, json)),
            ParseJson(expected_json));
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

TEST(JsonRpcResponseParserUnitTest, RPCResponse) {
  constexpr char json[] =
      R"({
       "jsonrpc": "2.0",
       "id": 1,
       "result": "hi"
     })";

  base::Value::Dict value = base::test::ParseJsonDict(json);
  auto response = json_rpc_responses::RPCResponse::FromValue(value);
  ASSERT_TRUE(response);
  EXPECT_EQ(response->jsonrpc, "2.0");
  EXPECT_EQ(response->id, base::Value(1));
  ASSERT_TRUE(response->result);
  EXPECT_EQ(*response->result, base::Value("hi"));
  EXPECT_FALSE(response->error);

  constexpr char error_json[] =
      R"({
       "jsonrpc": "2.0",
       "id": 2,
       "error": {
         "code":-32601,
         "message":"method does not exist"
       }
     })";
  value = base::test::ParseJsonDict(error_json);
  response = json_rpc_responses::RPCResponse::FromValue(value);
  ASSERT_TRUE(response);
  EXPECT_EQ(response->jsonrpc, "2.0");
  EXPECT_EQ(response->id, base::Value(2));
  EXPECT_EQ(response->result, std::nullopt);
  ASSERT_TRUE(response->error);
  EXPECT_EQ(response->error->code, -32601);
  EXPECT_EQ(response->error->message, "method does not exist");
}

TEST(JsonRpcResponseParserUnitTest, AnkrParseGetAccountBalanceResponse) {
  std::string json(R"(
    {
      "jsonrpc": "2.0",
      "id": 1,
      "result": {
        "totalBalanceUsd": "4915134435857.581297310767673907",
        "assets": [
          {
            "blockchain": "polygon",
            "tokenName": "Matic",
            "tokenSymbol": "MATIC",
            "tokenDecimals": "18",
            "tokenType": "NATIVE",
            "holderAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
            "balance": "120.275036899888325666",
            "balanceRawInteger": "120275036899888325666",
            "balanceUsd": "66.534394147826631446",
            "tokenPrice": "0.553185397924316979",
            "thumbnail": "polygon.svg"
          },
          {
            "blockchain": "polygon",
            "tokenName": "Malformed USDC",
            "tokenSymbol": "USDC",
            "tokenDecimals": "-6",
            "tokenType": "ERC20",
            "contractAddress": "0x2791bca1f2de4661ed88a30c99a7a9449aa84174",
            "holderAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
            "balance": "8.202765",
            "balanceRawInteger": "8202765",
            "balanceUsd": "8.202765",
            "tokenPrice": "1",
            "thumbnail": "usdc.png"
          },
          {
            "blockchain": "polygon",
            "tokenName": "USD Coin",
            "tokenSymbol": "USDC",
            "tokenDecimals": "6",
            "tokenType": "ERC20",
            "contractAddress": "0x2791bca1f2de4661ed88a30c99a7a9449aa84174",
            "holderAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
            "balance": "8.202765",
            "balanceRawInteger": "8202765",
            "balanceUsd": "8.202765",
            "tokenPrice": "1",
            "thumbnail": "usdc.png"
          },
          {
            "blockchain": "polygon",
            "tokenName": "Malformed USDC",
            "tokenSymbol": "USDC",
            "tokenDecimals": "6",
            "tokenType": "ERC20",
            "holderAddress": "0xa92d461a9a988a7f11ec285d39783a637fdd6ba4",
            "balance": "8.202765",
            "balanceRawInteger": "8202765",
            "balanceUsd": "8.202765",
            "tokenPrice": "1",
            "thumbnail": "usdc.png"
          }
        ]
      }
    }
  )");

  auto response = ankr::ParseGetAccountBalanceResponse(ParseJson(json));
  ASSERT_TRUE(response);
  ASSERT_EQ(response->size(), 2u);

  EXPECT_EQ(response->at(0)->asset->contract_address, "");
  EXPECT_EQ(response->at(0)->asset->name, "Matic");
  EXPECT_EQ(response->at(0)->asset->logo, "polygon.svg");
  EXPECT_FALSE(response->at(0)->asset->is_erc20);
  EXPECT_FALSE(response->at(0)->asset->is_erc721);
  EXPECT_FALSE(response->at(0)->asset->is_erc1155);
  EXPECT_FALSE(response->at(0)->asset->is_nft);
  EXPECT_FALSE(response->at(0)->asset->is_spam);
  EXPECT_EQ(response->at(0)->asset->symbol, "MATIC");
  EXPECT_EQ(response->at(0)->asset->decimals, 18);
  EXPECT_TRUE(response->at(0)->asset->visible);
  EXPECT_EQ(response->at(0)->asset->token_id, "");
  EXPECT_EQ(response->at(0)->asset->coingecko_id, "");
  EXPECT_EQ(response->at(0)->asset->chain_id, mojom::kPolygonMainnetChainId);
  EXPECT_EQ(response->at(0)->asset->coin, mojom::CoinType::ETH);
  EXPECT_EQ(response->at(0)->balance, "120275036899888325666");
  EXPECT_EQ(response->at(0)->formatted_balance, "120.275036899888325666");
  EXPECT_EQ(response->at(0)->balance_usd, "66.534394147826631446");
  EXPECT_EQ(response->at(0)->price_usd, "0.553185397924316979");

  EXPECT_EQ(response->at(1)->asset->contract_address,
            "0x2791bca1f2de4661ed88a30c99a7a9449aa84174");
  EXPECT_EQ(response->at(1)->asset->name, "USD Coin");
  EXPECT_EQ(response->at(1)->asset->logo, "usdc.png");
  EXPECT_TRUE(response->at(1)->asset->is_erc20);
  EXPECT_FALSE(response->at(1)->asset->is_erc721);
  EXPECT_FALSE(response->at(1)->asset->is_erc1155);
  EXPECT_FALSE(response->at(1)->asset->is_nft);
  EXPECT_FALSE(response->at(1)->asset->is_spam);
  EXPECT_EQ(response->at(1)->asset->symbol, "USDC");
  EXPECT_EQ(response->at(1)->asset->decimals, 6);
  EXPECT_TRUE(response->at(1)->asset->visible);
  EXPECT_EQ(response->at(1)->asset->token_id, "");
  EXPECT_EQ(response->at(1)->asset->coingecko_id, "");
  EXPECT_EQ(response->at(1)->asset->chain_id, mojom::kPolygonMainnetChainId);
  EXPECT_EQ(response->at(1)->asset->coin, mojom::CoinType::ETH);
  EXPECT_EQ(response->at(1)->balance, "8202765");
  EXPECT_EQ(response->at(1)->formatted_balance, "8.202765");
  EXPECT_EQ(response->at(1)->balance_usd, "8.202765");
  EXPECT_EQ(response->at(1)->price_usd, "1");
}

TEST(JsonRpcResponseParserUnitTest, GetUint64FromDictValue) {
  uint64_t ret;

  // Case 1: Successful conversion
  ret = 0;
  EXPECT_TRUE(GetUint64FromDictValue(
      base::test::ParseJsonDict(R"({"key": "18446744073709551615"})"), "key",
      false, &ret));
  EXPECT_EQ(ret, UINT64_MAX);

  // Case 2: ret is nullptr
  EXPECT_FALSE(GetUint64FromDictValue(
      base::test::ParseJsonDict(R"({"key": "18446744073709551615"})"), "key",
      false, nullptr));

  // Case 3: Key not found
  ret = 0;
  EXPECT_FALSE(GetUint64FromDictValue(
      base::test::ParseJsonDict(R"({"other_key": "18446744073709551615"})"),
      "key", false, &ret));

  // Case 4: Nullable and value is none
  ret = 0;
  EXPECT_TRUE(GetUint64FromDictValue(
      base::test::ParseJsonDict(R"({"key": null})"), "key", true, &ret));
  EXPECT_EQ(ret, 0U);

  // Case 5: Nullable but value is not none
  ret = 0;
  EXPECT_TRUE(GetUint64FromDictValue(
      base::test::ParseJsonDict(R"({"key": "0"})"), "key", true, &ret));
  EXPECT_EQ(ret, 0U);

  // Case 6: Non-string value
  ret = 0;
  EXPECT_FALSE(GetUint64FromDictValue(
      base::test::ParseJsonDict(R"({"key": 12345})"), "key", false, &ret));

  // Case 7: Empty string value
  ret = 0;
  EXPECT_FALSE(GetUint64FromDictValue(
      base::test::ParseJsonDict(R"({"key": ""})"), "key", false, &ret));

  // Case 8: Invalid string value
  ret = 0;
  EXPECT_FALSE(GetUint64FromDictValue(
      base::test::ParseJsonDict(R"({"key": "invalid"})"), "key", false, &ret));
}

TEST(JsonRpcResponseParserUnitTest, ConvertAllNumbersToString) {
  // OK: convert u64, f64, and i64 values to string
  std::string json(
      R"({"a":[{"key":18446744073709551615},{"key":-2},{"key":3.14}]})");
  EXPECT_EQ(
      ConvertAllNumbersToString("", json).value_or(""),
      R"({"a":[{"key":"18446744073709551615"},{"key":"-2"},{"key":"3.14"}]})");

  // OK: convert deeply nested value to string
  json = R"({"some":[{"deeply":{"nested":[{"path":123}]}}]})";
  EXPECT_EQ(ConvertAllNumbersToString("", json).value_or(""),
            R"({"some":[{"deeply":{"nested":[{"path":"123"}]}}]})");

  // OK: values other than u64/f64/i64 are unchanged
  json = R"({"a":[{"key":18446744073709551615},{"key":null},{"key":true}]})";
  EXPECT_EQ(
      ConvertAllNumbersToString("", json).value_or(""),
      R"({"a":[{"key":"18446744073709551615"},{"key":null},{"key":true}]})");

  // OK: empty object array, nothing to convert
  json = R"({"a":[]})";
  EXPECT_EQ(ConvertAllNumbersToString("", json).value_or(""), json);

  // OK: empty array json, nothing to convert
  json = R"([])";
  EXPECT_EQ(ConvertAllNumbersToString("", json).value_or(""), json);

  // OK: floating point values in scientific notation are unchanged
  json = R"({"a": 1.196568750220778e-7})";
  EXPECT_EQ(ConvertAllNumbersToString("", json).value_or(""),
            R"({"a":"1.196568750220778e-7"})");

  // OK: convert under specified JSON path only
  json = R"({"a":1,"outer":{"inner": 2}})";
  EXPECT_EQ(ConvertAllNumbersToString("/outer", json).value_or(""),
            R"({"a":1,"outer":{"inner":"2"}})");
  EXPECT_EQ(ConvertAllNumbersToString("/a", json).value_or(""),
            R"({"a":"1","outer":{"inner":2}})");

  // KO: invalid path has no effect on the JSON
  json = R"({"a":1,"outer":{"inner":2}})";
  EXPECT_EQ(ConvertAllNumbersToString("/invalid", json).value_or(""), json);
  EXPECT_EQ(ConvertAllNumbersToString("/", json).value_or(""), json);

  // KO: invalid cases
  std::vector<std::string> invalid_cases = {
      // invalid json
      R"({"a": hello})",
      // UINT64_MAX + 1
      R"("{a":[{"key":18446744073709551616}]})",
      // INT64_MIN
      R"("{a":[{"key":)" + base::NumberToString(INT64_MIN) + "}]}",
      // DBL_MIN
      R"("{a":[{"key":)" + base::NumberToString(DBL_MIN) + "}]}",
      // DBL_MAX
      R"("{a":[{"key":)" + base::NumberToString(DBL_MAX + 1) + "}]}"};
  for (const auto& invalid_case : invalid_cases) {
    EXPECT_EQ("", ConvertAllNumbersToString("", invalid_case).value_or(""))
        << invalid_case;
  }
}

}  // namespace brave_wallet
