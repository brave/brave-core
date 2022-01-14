/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/rpc_response_parser.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "components/grit/brave_components_strings.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {
TEST(RpcResponseParserUnitTest, ParseSingleStringResult) {
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

TEST(RpcResponseParserUnitTest, ParseBoolResult) {
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

TEST(RpcResponseParserUnitTest, ParseErrorResult) {
  mojom::ProviderError error;
  std::string error_message;
  std::string json =
      R"({
         "jsonrpc": "2.0",
         "id": 1,
         "error": {
           "code": -32000,
           "message": "transaction underpriced"
         }
       })";

  // kInvalidInput = -32000
  ParseErrorResult(json, &error, &error_message);
  EXPECT_EQ(error, mojom::ProviderError::kInvalidInput);
  EXPECT_EQ(error_message, "transaction underpriced");

  // No message should still work
  json =
      R"({
       "jsonrpc": "2.0",
       "id": 1,
       "error": {
         "code": -32000
       }
     })";
  ParseErrorResult(json, &error, &error_message);
  EXPECT_EQ(error, mojom::ProviderError::kInvalidInput);
  EXPECT_TRUE(error_message.empty());

  std::vector<std::string> errors{
      R"({
         "jsonrpc": "2.0",
         "id": 1,
         "error": {
           "message": "transaction underpriced"
         }
       })",
      R"({"jsonrpc": "2.0", "id": 1, "result": "0"})",
      R"({"jsonrpc": "2.0", "id": 1, "error": "0"})",
      R"({"jsonrpc": "2.0", "id": 1, "error": "0"})",
      R"({"jsonrpc": "2.0", "id": 1, "error": {}})",
      "some string",
  };

  for (const std::string& json : errors) {
    ParseErrorResult(json, &error, &error_message);
    EXPECT_EQ(error, mojom::ProviderError::kParsingError);
    EXPECT_EQ(error_message,
              l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
  }
}

}  // namespace brave_wallet
