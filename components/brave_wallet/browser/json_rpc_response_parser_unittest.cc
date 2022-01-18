/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "components/grit/brave_components_strings.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

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

  for (const std::string& json : errors) {
    ParseErrorResult<mojom::ProviderError>(json, &eth_error,
                                           &eth_error_message);
    EXPECT_EQ(eth_error, mojom::ProviderError::kParsingError);
    EXPECT_EQ(eth_error_message,
              l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));

    ParseErrorResult<mojom::SolanaProviderError>(json, &solana_error,
                                                 &solana_error_message);
    EXPECT_EQ(solana_error, mojom::SolanaProviderError::kParsingError);
    EXPECT_EQ(solana_error_message,
              l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

}  // namespace brave_wallet
