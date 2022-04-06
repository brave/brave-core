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

}  // namespace brave_wallet
