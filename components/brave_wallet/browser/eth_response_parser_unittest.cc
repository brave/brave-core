/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/eth_response_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(EthResponseParserUnitTest, ParseEthGetBalance) {
  std::string json(
      R"({
    "id":1,
    "jsonrpc": "2.0",
    "result": "0xe670ec64341771606e55d6b4ca35a1a6b75ee3d5145a99d05921026d1527331"
  })");
  std::string balance;
  ASSERT_TRUE(ParseEthGetBalance(json, &balance));
  ASSERT_EQ(
      balance,
      "0xe670ec64341771606e55d6b4ca35a1a6b75ee3d5145a99d05921026d1527331");
}

TEST(EthResponseParserUnitTest, ParseEthGetBalanceInvalidJSON) {
  std::string json("invalid JSON");
  std::string balance;
  ASSERT_FALSE(ParseEthGetBalance(json, &balance));
}

TEST(EthResponseParserUnitTest, ParseEthGetBalanceError) {
  std::string json(
      R"({
    code: 3,
    message: 'Error',
    data: []
  }")");
  std::string balance;
  ASSERT_FALSE(ParseEthGetBalance(json, &balance));
}

TEST(EthResponseParserUnitTest, ParseEthCall) {
  std::string json(
      R"({
    "id":1,
    "jsonrpc": "2.0",
    "result": "0x0"
  })");
  std::string result;
  ASSERT_TRUE(ParseEthGetBalance(json, &result));
  ASSERT_EQ(result, "0x0");
}

}  // namespace brave_wallet
