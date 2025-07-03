/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/eth_requests.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet::eth {

TEST(EthRequestUnitTest, eth_gasPrice) {
  ASSERT_EQ(eth_gasPrice(),
            R"({"id":1,"jsonrpc":"2.0","method":"eth_gasPrice","params":[]})");
}

TEST(EthRequestUnitTest, eth_blockNumber) {
  ASSERT_EQ(
      eth_blockNumber(),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_blockNumber","params":[]})");
}

TEST(EthRequestUnitTest, eth_feeHistory) {
  ASSERT_EQ(
      eth_feeHistory("0x28", "latest", std::vector<double>{20, 50, 80}),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_feeHistory","params":["0x28","latest",[20.0,50.0,80.0]]})");
}

TEST(EthRequestUnitTest, eth_getBalance) {
  ASSERT_EQ(
      eth_getBalance("0x407d73d8a49eeb85d32cf465507dd71d507100c1", "latest"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getBalance","params":["0x407d73d8a49eeb85d32cf465507dd71d507100c1","latest"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getTransactionCount) {
  ASSERT_EQ(
      eth_getTransactionCount("0x407d73d8a49eeb85d32cf465507dd71d507100c1",
                              "latest"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getTransactionCount","params":["0x407d73d8a49eeb85d32cf465507dd71d507100c1","latest"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getCode) {
  ASSERT_EQ(
      eth_getCode("0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b", "0x2"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getCode","params":["0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b","0x2"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_sendRawTransaction) {
  ASSERT_EQ(
      eth_sendRawTransaction("0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb97087"
                             "0f072445675058bb8eb970870f072445675"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_sendRawTransaction","params":["0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_call) {
  ASSERT_EQ(
      eth_call("0xb60e8dd61c5d32be8058bb8eb970870f07233155",
               "0xd46e8dd67c5d32be8058bb8eb970870f07244567", "0x76c0",
               "0x9184e72a000", "0x9184e72a",
               "0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058"
               "bb8eb970870f072445675",
               "latest"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_call","params":[{"data":"0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675","from":"0xb60e8dd61c5d32be8058bb8eb970870f07233155","gas":"0x76c0","gasPrice":"0x9184e72a000","to":"0xd46e8dd67c5d32be8058bb8eb970870f07244567","value":"0x9184e72a"},"latest"]})");  // NOLINT

  ASSERT_EQ(
      eth_call(
          "0xd46e8dd67c5d32be8058bb8eb970870f07244567",
          "0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_call","params":[{"data":"0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058","to":"0xd46e8dd67c5d32be8058bb8eb970870f07244567"},"latest"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_estimateGas) {
  ASSERT_EQ(
      eth_estimateGas("0xb60e8dd61c5d32be8058bb8eb970870f07233155",
                      "0xd46e8dd67c5d32be8058bb8eb970870f07244567", "0x76c0",
                      "0x9184e72a000", "0x9184e72a",
                      "0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f07244"
                      "5675058bb8eb970870f072445675"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_estimateGas","params":[{"data":"0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675","from":"0xb60e8dd61c5d32be8058bb8eb970870f07233155","gas":"0x76c0","gasPrice":"0x9184e72a000","to":"0xd46e8dd67c5d32be8058bb8eb970870f07244567","value":"0x9184e72a"}]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getBlockByNumber) {
  ASSERT_EQ(
      eth_getBlockByNumber("0x1b4", true),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getBlockByNumber","params":["0x1b4",true]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getTransactionReceipt) {
  ASSERT_EQ(
      eth_getTransactionReceipt(
          "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getTransactionReceipt","params":["0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getLogs) {
  base::Value::List topics;
  topics.Append(
      "0x000000000000000000000000a94f5374fce5edbc8e2a8697c15331677e6ebf0b");
  base::Value::List sub_topics;
  sub_topics.Append(
      "0x000000000000000000000000a94f5374fce5edbc8e2a8697c15331677e6ebf0b");
  sub_topics.Append(
      "0x0000000000000000000000000aff3454fce5edbc8cca8697c15331677e6ebccc");
  topics.Append(std::move(sub_topics));

  base::Value::List addresses;
  addresses.Append(base::Value("0x8888f1f195afa192cfee860698584c030f4c9db1"));

  base::Value::Dict filtering;
  filtering.Set("fromBlock", "0x1");
  filtering.Set("toBlock", "0x2");
  filtering.Set("address", std::move(addresses));
  filtering.Set("topics", std::move(topics));
  filtering.Set(
      "blockhash",
      "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238");
  ASSERT_EQ(
      eth_getLogs(std::move(filtering)),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getLogs","params":[{"address":["0x8888f1f195afa192cfee860698584c030f4c9db1"],"blockhash":"0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238","fromBlock":"0x1","toBlock":"0x2","topics":["0x000000000000000000000000a94f5374fce5edbc8e2a8697c15331677e6ebf0b",["0x000000000000000000000000a94f5374fce5edbc8e2a8697c15331677e6ebf0b","0x0000000000000000000000000aff3454fce5edbc8cca8697c15331677e6ebccc"]]}]})");  // NOLINT
}

}  // namespace brave_wallet::eth
