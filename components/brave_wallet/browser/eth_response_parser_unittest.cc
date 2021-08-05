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

TEST(EthResponseParserUnitTest, ParseEthGetBlockNumber) {
  const std::string json(R"({
    "id":83,
    "jsonrpc": "2.0",
    "result": "0x4b7" // 1207
  })");
  uint256_t block_num;
  ASSERT_TRUE(ParseEthGetBlockNumber(json, &block_num));
  EXPECT_EQ(block_num, uint256_t(1207));
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

TEST(EthResponseParserUnitTest, ParseEthGetTransactionReceipt) {
  std::string json(
      R"({
      "id": 1,
      "jsonrpc": "2.0",
      "result": {
        "transactionHash": "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238",
        "transactionIndex":  "0x1",
        "blockNumber": "0xb",
        "blockHash": "0xc6ef2fc5426d6ad6fd9e2a26abeab0aa2411b7ab17f30a99d3cb96aed1d1055b",
        "cumulativeGasUsed": "0x33bc",
        "gasUsed": "0x4dc",
        "contractAddress": "0xb60e8dd61c5d32be8058bb8eb970870f07233155",
        "logs": [],
        "logsBloom": "0x00...0",
        "status": "0x1"
      }
    })");
  auto receipt = mojom::TransactionReceipt::New();
  ASSERT_TRUE(ParseEthGetTransactionReceipt(json, &receipt));
  EXPECT_EQ(
      receipt->transaction_hash,
      "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238");
  EXPECT_EQ(receipt->transaction_index, "0x1");
  EXPECT_EQ(receipt->block_number, "0xb");
  EXPECT_EQ(
      receipt->block_hash,
      "0xc6ef2fc5426d6ad6fd9e2a26abeab0aa2411b7ab17f30a99d3cb96aed1d1055b");
  EXPECT_EQ(receipt->cumulative_gas_used, "0x33bc");
  EXPECT_EQ(receipt->gas_used, "0x4dc");
  EXPECT_EQ(receipt->contract_address,
            "0xb60e8dd61c5d32be8058bb8eb970870f07233155");
  EXPECT_EQ(receipt->logs_bloom, "0x00...0");
  EXPECT_TRUE(receipt->status);
}

TEST(EthResponseParserUnitTest, ParseEthGetTransactionReceiptNullContractAddr) {
  std::string json(
      R"({
      "id": 1,
      "jsonrpc": "2.0",
      "result": {
        "transactionHash": "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238",
        "transactionIndex":  "0x1",
        "blockNumber": "0xb",
        "blockHash": "0xc6ef2fc5426d6ad6fd9e2a26abeab0aa2411b7ab17f30a99d3cb96aed1d1055b",
        "cumulativeGasUsed": "0x33bc",
        "gasUsed": "0x4dc",
        "contractAddress": null,
        "logs": [],
        "logsBloom": "0x00...0",
        "status": "0x1"
      }
    })");
  auto receipt = mojom::TransactionReceipt::New();
  ASSERT_TRUE(ParseEthGetTransactionReceipt(json, &receipt));
  EXPECT_EQ(
      receipt->transaction_hash,
      "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238");
  EXPECT_EQ(receipt->transaction_index, "0x1");
  EXPECT_EQ(receipt->block_number, "0xb");
  EXPECT_EQ(
      receipt->block_hash,
      "0xc6ef2fc5426d6ad6fd9e2a26abeab0aa2411b7ab17f30a99d3cb96aed1d1055b");
  EXPECT_EQ(receipt->cumulative_gas_used, "0x33bc");
  EXPECT_EQ(receipt->gas_used, "0x4dc");
  EXPECT_EQ(receipt->contract_address, "");
  EXPECT_EQ(receipt->logs_bloom, "0x00...0");
  EXPECT_TRUE(receipt->status);
}

}  // namespace brave_wallet
