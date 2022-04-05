/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/eth_response_parser.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "components/grit/brave_components_strings.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {

namespace eth {

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
  TransactionReceipt receipt;
  ASSERT_TRUE(ParseEthGetTransactionReceipt(json, &receipt));
  EXPECT_EQ(
      receipt.transaction_hash,
      "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238");
  EXPECT_EQ(receipt.transaction_index, (uint256_t)1);
  EXPECT_EQ(receipt.block_number, (uint256_t)11);
  EXPECT_EQ(
      receipt.block_hash,
      "0xc6ef2fc5426d6ad6fd9e2a26abeab0aa2411b7ab17f30a99d3cb96aed1d1055b");
  EXPECT_EQ(receipt.cumulative_gas_used, (uint256_t)13244);
  EXPECT_EQ(receipt.gas_used, (uint256_t)1244);
  EXPECT_EQ(receipt.contract_address,
            "0xb60e8dd61c5d32be8058bb8eb970870f07233155");
  EXPECT_EQ(receipt.logs_bloom, "0x00...0");
  EXPECT_TRUE(receipt.status);
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
  TransactionReceipt receipt;
  ASSERT_TRUE(ParseEthGetTransactionReceipt(json, &receipt));
  EXPECT_EQ(
      receipt.transaction_hash,
      "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238");
  EXPECT_EQ(receipt.transaction_index, (uint256_t)1);
  EXPECT_EQ(receipt.block_number, (uint256_t)11);
  EXPECT_EQ(
      receipt.block_hash,
      "0xc6ef2fc5426d6ad6fd9e2a26abeab0aa2411b7ab17f30a99d3cb96aed1d1055b");
  EXPECT_EQ(receipt.cumulative_gas_used, (uint256_t)13244);
  EXPECT_EQ(receipt.gas_used, (uint256_t)1244);
  EXPECT_EQ(receipt.contract_address, "");
  EXPECT_EQ(receipt.logs_bloom, "0x00...0");
  EXPECT_TRUE(receipt.status);
}

TEST(EthResponseParserUnitTest, ParseAddressResult) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x0000000000000000000000004976fb03c32e5b8cfe2b6ccb31c09ba78ebaba41\"}";
  std::string addr;
  EXPECT_TRUE(ParseAddressResult(json, &addr));
  // Will be converted to checksum address.
  EXPECT_EQ(addr, "0x4976fb03C32e5B8cfe2b6cCB31c09Ba78EBaBa41");

  // Non-expected address size.
  addr = "";
  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x0000000000000000000000004976fb03c32e5b8cfe2b6ccb31c09ba78eba\"}";
  EXPECT_FALSE(ParseAddressResult(json, &addr));
  EXPECT_TRUE(addr.empty());
}

TEST(EthResponseParserUnitTest, ParseEnsResolverContentHash) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x00000000000000000000000000000000000000000000000000000000000000200000"
      "00"
      "0000000000000000000000000000000000000000000000000000000026e3010170122023"
      "e0160eec32d7875c19c5ac7c03bc1f306dc260080d621454bc5f631e7310a70000000000"
      "000000000000000000000000000000000000000000\"}";
  std::string content_hash;
  EXPECT_TRUE(ParseEnsResolverContentHash(json, &content_hash));
  EXPECT_EQ(
      ipfs::ContentHashToCIDv1URL(content_hash).spec(),
      "ipfs://bafybeibd4ala53bs26dvygofvr6ahpa7gbw4eyaibvrbivf4l5rr44yqu4");

  content_hash = "";
  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x000000000000000000000000000000000000000000000000000000000000002000";

  EXPECT_FALSE(ParseEnsResolverContentHash(json, &content_hash));
  EXPECT_TRUE(content_hash.empty());
}

TEST(EthResponseParserUnitTest, ParseUnstoppableDomainsProxyReaderGetMany) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      // offset for array
      "\"0x0000000000000000000000000000000000000000000000000000000000000020"
      // count for array
      "0000000000000000000000000000000000000000000000000000000000000006"
      // offsets for array elements
      "00000000000000000000000000000000000000000000000000000000000000c0"
      "0000000000000000000000000000000000000000000000000000000000000120"
      "0000000000000000000000000000000000000000000000000000000000000180"
      "00000000000000000000000000000000000000000000000000000000000001a0"
      "00000000000000000000000000000000000000000000000000000000000001c0"
      "0000000000000000000000000000000000000000000000000000000000000200"
      // count for "QmWrdNJWMbvRxxzLhojVKaBDswS4KNVM7LvjsN7QbDrvka"
      "000000000000000000000000000000000000000000000000000000000000002e"
      // encoding for "QmWrdNJWMbvRxxzLhojVKaBDswS4KNVM7LvjsN7QbDrvka"
      "516d5772644e4a574d62765278787a4c686f6a564b614244737753344b4e564d"
      "374c766a734e3751624472766b61000000000000000000000000000000000000"
      // count for "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR"
      "000000000000000000000000000000000000000000000000000000000000002e"
      // encoding for "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR"
      "516d6257717842454b433350387471734b633938786d574e7a727a4474524c4d"
      "694d504c387742755447734d6e52000000000000000000000000000000000000"
      // count for empty dns.A
      "0000000000000000000000000000000000000000000000000000000000000000"
      // count for empty dns.AAAA
      "0000000000000000000000000000000000000000000000000000000000000000"
      // count for "https://fallback1.test.com"
      "000000000000000000000000000000000000000000000000000000000000001a"
      // encoding for "https://fallback1.test.com"
      "68747470733a2f2f66616c6c6261636b312e746573742e636f6d000000000000"
      // count for "https://fallback2.test.com"
      "000000000000000000000000000000000000000000000000000000000000001a"
      // encoding for "https://fallback2.test.com"
      "68747470733a2f2f66616c6c6261636b322e746573742e636f6d000000000000\"}";

  std::vector<std::string> expected_values = {
      "QmWrdNJWMbvRxxzLhojVKaBDswS4KNVM7LvjsN7QbDrvka",  // dweb.ipfs.hash
      "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR",  // ipfs.html.value
      "",                                                // dns.A
      "",                                                // dns.AAAA
      "https://fallback1.test.com",                      // browser.redirect_url
      "https://fallback2.test.com",  // ipfs.redirect_domain.value
  };

  std::vector<std::string> values;
  EXPECT_TRUE(ParseUnstoppableDomainsProxyReaderGetMany(json, &values));
  EXPECT_EQ(values, expected_values);

  values.clear();
  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x000000000000000000000000000000000000000000000000000000000000002000";
  EXPECT_FALSE(ParseUnstoppableDomainsProxyReaderGetMany(json, &values));
  EXPECT_TRUE(values.empty());
}

TEST(EthResponseParserUnitTest, ParseUnstoppableDomainsProxyReaderGet) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      // offset to string
      "\"0x0000000000000000000000000000000000000000000000000000000000000020"
      // string len
      "000000000000000000000000000000000000000000000000000000000000002a"
      // Encoded string of 0x8aaD44321A86b170879d7A244c1e8d360c99DdA8
      "3078386161443434333231413836623137303837396437413234346331653864"
      "3336306339394464413800000000000000000000000000000000000000000000\"}";
  std::string value;
  EXPECT_TRUE(ParseUnstoppableDomainsProxyReaderGet(json, &value));
  EXPECT_EQ(value, "0x8aaD44321A86b170879d7A244c1e8d360c99DdA8");

  value = "";
  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x000000000000000000000000000000000000000000000000000000000000002000";
  EXPECT_FALSE(ParseUnstoppableDomainsProxyReaderGet(json, &value));
  EXPECT_TRUE(value.empty());
}

TEST(EthResponseParserUnitTest, ParseEthGetFeeHistory) {
  std::string json =
      R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result": {
          "baseFeePerGas": [
            "0x257093e880",
            "0x20f4138789",
            "0x20b04643ea",
            "0x1da8692acc",
            "0x215d00b8c8",
            "0x24beaded75"
          ],
          "gasUsedRatio": [
            0.020687709938714324,
            0.4678514936136911,
            0.12914042746424212,
            0.999758,
            0.9054214892490816
          ],
          "oldestBlock": "0xd6b1b0",
          "reward": [
            [
              "0x77359400",
              "0x77359400",
              "0x3a3eb2ac0"
            ],
            [
              "0x59682f00",
              "0x77359400",
              "0x48ae2f980"
            ],
            [
              "0x59682f00",
              "0x9502f900",
              "0x17d1ffc7d6"
            ],
            [
              "0xee6b2800",
              "0x32bd81734",
              "0xda2b71b34"
            ],
            [
              "0x77359400",
              "0x77359400",
              "0x2816a6cfb"
            ]
          ]
        }
      })";

  std::vector<std::string> base_fee_per_gas;
  std::vector<double> gas_used_ratio;
  std::string oldest_block;
  std::vector<std::vector<std::string>> reward;
  EXPECT_TRUE(ParseEthGetFeeHistory(json, &base_fee_per_gas, &gas_used_ratio,
                                    &oldest_block, &reward));
  EXPECT_EQ(base_fee_per_gas,
            (std::vector<std::string>{"0x257093e880", "0x20f4138789",
                                      "0x20b04643ea", "0x1da8692acc",
                                      "0x215d00b8c8", "0x24beaded75"}));
  EXPECT_EQ(
      gas_used_ratio,
      (std::vector<double>{0.020687709938714324, 0.4678514936136911,
                           0.12914042746424212, 0.999758, 0.9054214892490816}));
  EXPECT_EQ(oldest_block, "0xd6b1b0");
  EXPECT_EQ(reward, (std::vector<std::vector<std::string>>{
                        {"0x77359400", "0x77359400", "0x3a3eb2ac0"},
                        {"0x59682f00", "0x77359400", "0x48ae2f980"},
                        {"0x59682f00", "0x9502f900", "0x17d1ffc7d6"},
                        {"0xee6b2800", "0x32bd81734", "0xda2b71b34"},
                        {"0x77359400", "0x77359400", "0x2816a6cfb"}}));

  // Empty result for the correct schema parses OK
  json = R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result": {
          "baseFeePerGas": [],
          "gasUsedRatio": [],
          "oldestBlock": "0xd6b1b0",
          "reward": []
        }
      })";
  base_fee_per_gas.clear();
  gas_used_ratio.clear();
  oldest_block.clear();
  reward.clear();
  EXPECT_TRUE(ParseEthGetFeeHistory(json, &base_fee_per_gas, &gas_used_ratio,
                                    &oldest_block, &reward));
  EXPECT_EQ(base_fee_per_gas, std::vector<std::string>());
  EXPECT_EQ(gas_used_ratio, std::vector<double>());
  EXPECT_EQ(oldest_block, "0xd6b1b0");
  EXPECT_EQ(reward, std::vector<std::vector<std::string>>());

  // Missing reward is OK because it isn't specified when percentiles param
  // isn't
  json = R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result": {
          "baseFeePerGas": [],
          "gasUsedRatio": [],
          "oldestBlock": "0xd6b1b0"
        }
      })";
  base_fee_per_gas.clear();
  gas_used_ratio.clear();
  oldest_block.clear();
  reward.clear();
  EXPECT_TRUE(ParseEthGetFeeHistory(json, &base_fee_per_gas, &gas_used_ratio,
                                    &oldest_block, &reward));
  EXPECT_EQ(base_fee_per_gas, std::vector<std::string>());
  EXPECT_EQ(gas_used_ratio, std::vector<double>());
  EXPECT_EQ(oldest_block, "0xd6b1b0");
  EXPECT_EQ(reward, std::vector<std::vector<std::string>>());

  // Unexpected input
  EXPECT_FALSE(ParseEthGetFeeHistory("", &base_fee_per_gas, &gas_used_ratio,
                                     &oldest_block, &reward));
  EXPECT_FALSE(ParseEthGetFeeHistory("3", &base_fee_per_gas, &gas_used_ratio,
                                     &oldest_block, &reward));
  EXPECT_FALSE(ParseEthGetFeeHistory("{", &base_fee_per_gas, &gas_used_ratio,
                                     &oldest_block, &reward));
  EXPECT_FALSE(ParseEthGetFeeHistory("{}", &base_fee_per_gas, &gas_used_ratio,
                                     &oldest_block, &reward));

  // Invalid reward input
  json = R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result": {
          "baseFeePerGas": [],
          "gasUsedRatio": [],
          "oldestBlock": "0xd6b1b0",
          "reward": [[3]]
        }
      })";
  EXPECT_FALSE(ParseEthGetFeeHistory(json, &base_fee_per_gas, &gas_used_ratio,
                                     &oldest_block, &reward));

  // Invalid oldest block type
  json = R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result": {
          "baseFeePerGas": [],
          "gasUsedRatio": [],
          "oldestBlock": 3,
          "reward": [[]]
        }
      })";
  EXPECT_FALSE(ParseEthGetFeeHistory(json, &base_fee_per_gas, &gas_used_ratio,
                                     &oldest_block, &reward));

  // Invalid used ratio value
  json = R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result": {
          "baseFeePerGas": [],
          "gasUsedRatio": ["3"],
          "oldestBlock": "0xd6b1b0",
          "reward": [[]]
        }
      })";
  EXPECT_FALSE(ParseEthGetFeeHistory(json, &base_fee_per_gas, &gas_used_ratio,
                                     &oldest_block, &reward));

  // Invalid base fee type
  json = R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result": {
          "baseFeePerGas": [3],
          "gasUsedRatio": [],
          "oldestBlock": "0xd6b1b0",
          "reward": [[]]
        }
      })";
  EXPECT_FALSE(ParseEthGetFeeHistory(json, &base_fee_per_gas, &gas_used_ratio,
                                     &oldest_block, &reward));
}

TEST(EthResponseParserUnitTest, ParseDataURIAndExtractJSON) {
  std::string json;
  std::string url;
  // Invalid URL
  EXPECT_FALSE(ParseDataURIAndExtractJSON(GURL(""), &json));
  // Valid URL, incorrect scheme
  EXPECT_FALSE(ParseDataURIAndExtractJSON(GURL("https://brave.com"),
                                          &json));  // Incorrect scheme
  // Valid URL and scheme, invalid mime_type
  EXPECT_FALSE(ParseDataURIAndExtractJSON(
      GURL("data:text/vnd-example+xyz;foo=bar;base64,R0lGODdh"),
      &json));  // Incorrect mime type

  // All valid
  std::string expected =
      R"({"attributes":"","description":"Non fungible lion","image":"data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCA1MDAgNTAwIj48cGF0aCBkPSIiLz48L3N2Zz4=","name":"NFL"})";
  url =
      "data:application/"
      "json;base64,"
      "eyJhdHRyaWJ1dGVzIjoiIiwiZGVzY3JpcHRpb24iOiJOb24gZnVuZ2libGUgbGlvbiIsImlt"
      "YWdlIjoiZGF0YTppbWFnZS9zdmcreG1sO2Jhc2U2NCxQSE4yWnlCNGJXeHVjejBpYUhSMGNE"
      "b3ZMM2QzZHk1M015NXZjbWN2TWpBd01DOXpkbWNpSUhacFpYZENiM2c5SWpBZ01DQTFNREFn"
      "TlRBd0lqNDhjR0YwYUNCa1BTSWlMejQ4TDNOMlp6ND0iLCJuYW1lIjoiTkZMIn0=";
  EXPECT_TRUE(ParseDataURIAndExtractJSON(GURL(url), &json));
  EXPECT_EQ(json, expected);
}

}  // namespace eth

}  // namespace brave_wallet
