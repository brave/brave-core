/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_response_parser.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/test/values_test_util.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "components/grit/brave_components_strings.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

using base::test::ParseJson;

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
  ASSERT_TRUE(ParseEthGetBalance(ParseJson(json), &balance));
  ASSERT_EQ(
      balance,
      "0xe670ec64341771606e55d6b4ca35a1a6b75ee3d5145a99d05921026d1527331");
}

TEST(EthResponseParserUnitTest, ParseEthGetBalanceInvalidJSON) {
  std::string balance;
  ASSERT_FALSE(ParseEthGetBalance(base::Value(), &balance));
}

TEST(EthResponseParserUnitTest, ParseEthGetBalanceError) {
  std::string json(
      R"({
    "code": 3,
    "message": "Error",
    "data": []
  })");
  std::string balance;
  ASSERT_FALSE(ParseEthGetBalance(ParseJson(json), &balance));
}

TEST(EthResponseParserUnitTest, ParseEthGetBlockNumber) {
  const std::string json(R"({
    "id":83,
    "jsonrpc": "2.0",
    "result": "0x4b7" // 1207
  })");
  uint256_t block_num;
  ASSERT_TRUE(ParseEthGetBlockNumber(ParseJson(json), &block_num));
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
  ASSERT_EQ(ParseEthCall(ParseJson(json)), "0x0");
}

TEST(EthResponseParserUnitTest, DecodeEthCallResponse) {
  // OK: 32-byte uint256
  std::string result =
      "0x00000000000000000000000000000000000000000000000166e12cfce39a0000";
  auto args = DecodeEthCallResponse(result, {"uint256"});
  ASSERT_NE(args, std::nullopt);
  ASSERT_EQ(args->size(), 1UL);
  ASSERT_EQ(args->at(0), "0x166e12cfce39a0000");

  // OK: 32-byte uint256 with extra zero bytes
  result =
      "0x0000000000000000000000000000000000000000000000000000000000045d12000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "00000000000000000000000000000000000000000000000000";
  args = DecodeEthCallResponse(result, {"uint256"});
  ASSERT_NE(args, std::nullopt);
  ASSERT_EQ(args->size(), 1UL);
  ASSERT_EQ(args->at(0), "0x45d12");

  // KO: insufficient length of response
  ASSERT_EQ(DecodeEthCallResponse("0x0", {"uint256"}), std::nullopt);

  // KO: invalid response
  ASSERT_EQ(DecodeEthCallResponse("foobarbaz", {"uint256"}), std::nullopt);
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
  ASSERT_TRUE(ParseEthGetTransactionReceipt(ParseJson(json), &receipt));
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
  ASSERT_TRUE(ParseEthGetTransactionReceipt(ParseJson(json), &receipt));
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
  EXPECT_TRUE(ParseAddressResult(ParseJson(json), &addr));
  // Will be converted to checksum address.
  EXPECT_EQ(addr, "0x4976fb03C32e5B8cfe2b6cCB31c09Ba78EBaBa41");

  // Non-expected address size.
  addr = "";
  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x0000000000000000000000004976fb03c32e5b8cfe2b6ccb31c09ba78eba\"}";
  EXPECT_FALSE(ParseAddressResult(ParseJson(json), &addr));
  EXPECT_TRUE(addr.empty());
}

TEST(EthResponseParserUnitTest, ParseEthGetLogs) {
  std::vector<Log> logs;
  std::string json(R"({
    "jsonrpc": "2.0",
    "id": 1,
    "result": [
      {
        "address": "0x6b175474e89094c44da98b954eedeac495271d0f",
        "blockHash": "0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber": "0xd6464c",
        "data": "0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex": "0x159",
        "removed": false,
        "topics": [
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
        ],
        "transactionHash": "0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex": "0x9f"
      }
    ]
  })");
  EXPECT_TRUE(ParseEthGetLogs(ParseJson(json), &logs));
  EXPECT_EQ(logs[0].address, "0x6b175474e89094c44da98b954eedeac495271d0f");
  EXPECT_EQ(
      logs[0].block_hash,
      "0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e");
  EXPECT_EQ(logs[0].block_number, static_cast<uint256_t>(14042700));
  EXPECT_EQ(
      logs[0].data,
      "0x00000000000000000000000000000000000000000000000555aff1f0fae8c000");
  EXPECT_EQ(logs[0].log_index, static_cast<uint256_t>(345));
  EXPECT_EQ(logs[0].removed, false);
  EXPECT_EQ(
      logs[0].transaction_hash,
      "0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066");
  std::vector<std::string> expected_topics = {
      "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
      "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
      "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"};
  ASSERT_EQ(logs[0].topics.size(), expected_topics.size());
  for (size_t i = 0; i < expected_topics.size(); ++i) {
    EXPECT_EQ(logs[0].topics[i], expected_topics[i]);
  }
  EXPECT_EQ(logs[0].transaction_index, static_cast<uint32_t>(159));

  // Invalid JSON
  EXPECT_FALSE(ParseEthGetLogs(base::Value(), &logs));

  // Missing address
  json = R"({
    "jsonrpc": "2.0",
    "id": 1,
    "result": [
      {
        "blockHash": "0x2961ceb6c16bab72a55f79e394a35f2bf1c62b30446e3537280f7c22c3115e6e",
        "blockNumber": "0xd6464c",
        "data": "0x00000000000000000000000000000000000000000000000555aff1f0fae8c000",
        "logIndex": "0x159",
        "removed": false,
        "topics": [
          "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
          "0x000000000000000000000000503828976d22510aad0201ac7ec88293211d23da",
          "0x000000000000000000000000b4b2802129071b2b9ebb8cbb01ea1e4d14b34961"
        ],
        "transactionHash": "0x2e652b70966c6a05f4b3e68f20d6540b7a5ab712385464a7ccf62774d39b7066",
        "transactionIndex": "0x9f"
      }
    ]
  })";

  EXPECT_FALSE(ParseEthGetLogs(ParseJson(json), &logs));
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

  auto values = ParseUnstoppableDomainsProxyReaderGetMany(ParseJson(json));
  ASSERT_TRUE(values);
  EXPECT_EQ(values, expected_values);

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x000000000000000000000000000000000000000000000000000000000000002000\""
      "}";
  values = ParseUnstoppableDomainsProxyReaderGetMany(ParseJson(json));
  ASSERT_FALSE(values);
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
  auto value = ParseUnstoppableDomainsProxyReaderGet(ParseJson(json));
  EXPECT_TRUE(value);
  EXPECT_EQ(value, "0x8aaD44321A86b170879d7A244c1e8d360c99DdA8");

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"0x000000000000000000000000000000000000000000000000000000000000002000\""
      "}";
  value = ParseUnstoppableDomainsProxyReaderGet(ParseJson(json));
  EXPECT_FALSE(value);
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
            "0.020687709938714324",
            "0.4678514936136911",
            "0.12914042746424212",
            "0.999758",
            "0.9054214892490816"
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
  EXPECT_TRUE(ParseEthGetFeeHistory(ParseJson(json), &base_fee_per_gas,
                                    &gas_used_ratio, &oldest_block, &reward));
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
    }
  )";
  base_fee_per_gas.clear();
  gas_used_ratio.clear();
  oldest_block.clear();
  reward.clear();
  EXPECT_TRUE(ParseEthGetFeeHistory(ParseJson(json), &base_fee_per_gas,
                                    &gas_used_ratio, &oldest_block, &reward));
  EXPECT_EQ(base_fee_per_gas, std::vector<std::string>());
  EXPECT_EQ(gas_used_ratio, std::vector<double>());
  EXPECT_EQ(oldest_block, "0xd6b1b0");
  EXPECT_EQ(reward, std::vector<std::vector<std::string>>());

  // Integer values in gasUsedRatio should be handled correctly
  json = R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result": {
          "baseFeePerGas": [],
          "gasUsedRatio": ["1", "0"],
          "oldestBlock": "0xd6b1b0"
        }
      })";
  base_fee_per_gas.clear();
  gas_used_ratio.clear();
  oldest_block.clear();
  reward.clear();
  EXPECT_TRUE(ParseEthGetFeeHistory(ParseJson(json), &base_fee_per_gas,
                                    &gas_used_ratio, &oldest_block, &reward));
  EXPECT_EQ(base_fee_per_gas, std::vector<std::string>());
  EXPECT_EQ(gas_used_ratio, (std::vector<double>{1.0, 0.0}));
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
  EXPECT_TRUE(ParseEthGetFeeHistory(ParseJson(json), &base_fee_per_gas,
                                    &gas_used_ratio, &oldest_block, &reward));
  EXPECT_EQ(base_fee_per_gas, std::vector<std::string>());
  EXPECT_EQ(gas_used_ratio, std::vector<double>());
  EXPECT_EQ(oldest_block, "0xd6b1b0");
  EXPECT_EQ(reward, std::vector<std::vector<std::string>>());

  // null reward is OK which is treated the same as missing reward
  json = R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result": {
          "baseFeePerGas": [],
          "gasUsedRatio": [],
          "oldestBlock": "0xd6b1b0",
          "reward": null
        }
      })";
  base_fee_per_gas.clear();
  gas_used_ratio.clear();
  oldest_block.clear();
  reward.clear();
  EXPECT_TRUE(ParseEthGetFeeHistory(ParseJson(json), &base_fee_per_gas,
                                    &gas_used_ratio, &oldest_block, &reward));
  EXPECT_EQ(base_fee_per_gas, std::vector<std::string>());
  EXPECT_EQ(gas_used_ratio, std::vector<double>());
  EXPECT_EQ(oldest_block, "0xd6b1b0");
  EXPECT_EQ(reward, std::vector<std::vector<std::string>>());

  // Unexpected input
  EXPECT_FALSE(ParseEthGetFeeHistory(base::Value(), &base_fee_per_gas,
                                     &gas_used_ratio, &oldest_block, &reward));
  EXPECT_FALSE(ParseEthGetFeeHistory(ParseJson("3"), &base_fee_per_gas,
                                     &gas_used_ratio, &oldest_block, &reward));
  EXPECT_FALSE(ParseEthGetFeeHistory(ParseJson("{}"), &base_fee_per_gas,
                                     &gas_used_ratio, &oldest_block, &reward));

  // Invalid reward input
  json = R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result": {
          "baseFeePerGas": [],
          "gasUsedRatio": [],
          "oldestBlock": "0xd6b1b0",
          "reward": [[true]]
        }
      })";
  EXPECT_FALSE(ParseEthGetFeeHistory(ParseJson(json), &base_fee_per_gas,
                                     &gas_used_ratio, &oldest_block, &reward));

  // Invalid oldest block type
  json = R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result": {
          "baseFeePerGas": [],
          "gasUsedRatio": [],
          "oldestBlock": true,
          "reward": [[]]
        }
      })";
  EXPECT_FALSE(ParseEthGetFeeHistory(ParseJson(json), &base_fee_per_gas,
                                     &gas_used_ratio, &oldest_block, &reward));

  // Invalid used ratio value
  json = R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result": {
          "baseFeePerGas": [],
          "gasUsedRatio": ["abc"],
          "oldestBlock": "0xd6b1b0",
          "reward": [[]]
        }
      })";
  EXPECT_FALSE(ParseEthGetFeeHistory(ParseJson(json), &base_fee_per_gas,
                                     &gas_used_ratio, &oldest_block, &reward));

  // Invalid base fee type
  json = R"(
      {
        "jsonrpc":"2.0",
        "id":1,
        "result": {
          "baseFeePerGas": [true],
          "gasUsedRatio": [],
          "oldestBlock": "0xd6b1b0",
          "reward": [[]]
        }
      })";
  EXPECT_FALSE(ParseEthGetFeeHistory(ParseJson(json), &base_fee_per_gas,
                                     &gas_used_ratio, &oldest_block, &reward));
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

TEST(EthResponseParserUnitTest, ParseTokenUri) {
  GURL url;

  // Valid (3 total)
  // (1/3) Valid IPFS URLs
  std::string body = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000003a697066733a2f2f516d65536a53696e4870506e6d586d73704d6a776958794e367a533445397a63636172694752336a7863615774712f31383137000000000000"
  })";
  EXPECT_TRUE(eth::ParseTokenUri(ParseJson(body), &url));
  EXPECT_EQ(url.spec(),
            "ipfs://QmeSjSinHpPnmXmspMjwiXyN6zS4E9zccariGR3jxcaWtq/1817");

  // (2/3) Data URIs are parsed
  body = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x00000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000135646174613a6170706c69636174696f6e2f6a736f6e3b6261736536342c65794a686448527961574a316447567a496a6f69496977695a47567a59334a7063485270623234694f694a4f623234675a6e56755a326c696247556762476c7662694973496d6c745957646c496a6f695a474630595470706257466e5a53397a646d6372654731734f324a68633255324e43785153453479576e6c434e474a586548566a656a4270595568534d474e4562335a4d4d32517a5a486b314d3031354e585a6a62574e3254577042643031444f58706b62574e7053556861634670595a454e694d326335535770425a3031445154464e5245466e546c524264306c714e44686a5230597759554e436131425453576c4d656a513454444e4f4d6c70364e4430694c434a755957316c496a6f69546b5a4d496e303d0000000000000000000000"
  })";
  EXPECT_TRUE(eth::ParseTokenUri(ParseJson(body), &url));
  EXPECT_EQ(
      url.spec(),
      R"(data:application/json;base64,eyJhdHRyaWJ1dGVzIjoiIiwiZGVzY3JpcHRpb24iOiJOb24gZnVuZ2libGUgbGlvbiIsImltYWdlIjoiZGF0YTppbWFnZS9zdmcreG1sO2Jhc2U2NCxQSE4yWnlCNGJXeHVjejBpYUhSMGNEb3ZMM2QzZHk1M015NXZjbWN2TWpBd01DOXpkbWNpSUhacFpYZENiM2c5SWpBZ01DQTFNREFnTlRBd0lqNDhjR0YwYUNCa1BTSWlMejQ4TDNOMlp6ND0iLCJuYW1lIjoiTkZMIn0=)");

  // (3/3) HTTP URLs are parsed
  body = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000002468747470733a2f2f696e76697369626c65667269656e64732e696f2f6170692f3138313700000000000000000000000000000000000000000000000000000000"
  })";
  EXPECT_TRUE(eth::ParseTokenUri(ParseJson(body), &url));
  EXPECT_EQ(url.spec(), "https://invisiblefriends.io/api/1817");

  // Invalid (2 total)
  // (1/2) Invalid provider response returns false
  url = GURL();
  body = R"({
   "jsonrpc":"2.0",
   "id":1,
   "error": {
     "code":-32005,
     "message": "Request exceeds defined limit"
   }
 })";
  EXPECT_FALSE(eth::ParseTokenUri(ParseJson(body), &url));
  EXPECT_EQ(url.spec(), "");

  // (2/2) Invalid URL returns false (https//brave.com)
  body = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x0000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001068747470732f2f62726176652e636f6d00000000000000000000000000000000"
  })";
  EXPECT_FALSE(eth::ParseTokenUri(ParseJson(body), &url));
  EXPECT_EQ(url.spec(), "");
}

TEST(EthResponseParserUnitTest, ParseStringResult) {
  std::string value;

  // Valid
  std::string json = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result": "0x000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73000000000000000000000000000000000000000000000000000000"
  })";
  EXPECT_TRUE(eth::ParseStringResult(ParseJson(json), &value));
  EXPECT_EQ(
      value,
      "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks");

  // Invalid JSON
  value = "";
  EXPECT_FALSE(eth::ParseStringResult(base::Value(), &value));
  EXPECT_TRUE(value.empty());

  // Valid JSON, invalid result (too short)
  value = "";
  json = R"({
      "jsonrpc":"2.0",
      "id":1,
      "result":"0x00000000000000000000000000000007"
  })";
  EXPECT_FALSE(eth::ParseStringResult(ParseJson(json), &value));
  EXPECT_TRUE(value.empty());
}

TEST(EthResponseParserUnitTest, DecodeGetERC20TokenBalancesEthCallResponse) {
  std::optional<std::vector<std::optional<std::string>>> result;

  // Empty string returns null
  result = eth::DecodeGetERC20TokenBalancesEthCallResponse("");
  EXPECT_FALSE(result.has_value());

  // Invalid ((bool, bytes))[] response returns null
  result = eth::DecodeGetERC20TokenBalancesEthCallResponse(
      "0x00000000000000000000000000000000000000000000000166e12cfce39a0000");
  EXPECT_FALSE(result.has_value());

  // Valid ((bool, bytes))[] response returns vector of 2 elements
  result = eth::DecodeGetERC20TokenBalancesEthCallResponse(
      "0x0000000000000000000000000000000000000000000000000000000000000020000000"
      "000000000000000000000000000000000000000000000000000000000300000000000000"
      "000000000000000000000000000000000000000000000000600000000000000000000000"
      "0000000000000000000000000000000000000000e0000000000000000000000000000000"
      "000000000000000000000000000000014000000000000000000000000000000000000000"
      "000000000000000000000000010000000000000000000000000000000000000000000000"
      "000000000000000040000000000000000000000000000000000000000000000000000000"
      "0000000020000000000000000000000000000000000000000000000006e83695ab1f893c"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000000000000000000004000000000000000"
      "000000000000000000000000000000000000000000000000000000000000000000000000"
      "000000000000000000000000000000000000000001000000000000000000000000000000"
      "000000000000000000000000000000004000000000000000000000000000000000000000"
      "000000000000000000000000200000000000000000000000000000000000000000000000"
      "000000000000000000");
  EXPECT_TRUE(result.has_value());
  EXPECT_EQ(result.value().size(), 3u);

  EXPECT_EQ(
      result.value()[0].value(),
      "0x000000000000000000000000000000000000000000000006e83695ab1f893c00");
  EXPECT_FALSE(result.value()[1].has_value());
  EXPECT_EQ(
      result.value()[2].value(),
      "0x0000000000000000000000000000000000000000000000000000000000000000");
}

}  // namespace eth

}  // namespace brave_wallet
