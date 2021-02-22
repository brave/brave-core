/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/eth_requests.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(EthRequestUnitTest, web3_clientVersion) {
  ASSERT_EQ(
      web3_clientVersion(),
      R"({"id":1,"jsonrpc":"2.0","method":"web3_clientVersion","params":[]})");
}

TEST(EthRequestUnitTest, web3_sha3) {
  ASSERT_EQ(
      web3_sha3("hello world"),
      R"({"id":1,"jsonrpc":"2.0","method":"web3_sha3","params":["0x68656c6c6f20776f726c64"]})");
}

TEST(EthRequestUnitTest, net_version) {
  ASSERT_EQ(net_version(),
            R"({"id":1,"jsonrpc":"2.0","method":"net_version","params":[]})");
}

TEST(EthRequestUnitTest, net_listening) {
  ASSERT_EQ(net_listening(),
            R"({"id":1,"jsonrpc":"2.0","method":"net_listening","params":[]})");
}

TEST(EthRequestUnitTest, net_peerCount) {
  ASSERT_EQ(net_peerCount(),
            R"({"id":1,"jsonrpc":"2.0","method":"net_peerCount","params":[]})");
}

TEST(EthRequestUnitTest, eth_protocolVersion) {
  ASSERT_EQ(
      eth_protocolVersion(),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_protocolVersion","params":[]})");
}

TEST(EthRequestUnitTest, eth_syncing) {
  ASSERT_EQ(eth_syncing(),
            R"({"id":1,"jsonrpc":"2.0","method":"eth_syncing","params":[]})");
}

TEST(EthRequestUnitTest, eth_coinbase) {
  ASSERT_EQ(eth_coinbase(),
            R"({"id":1,"jsonrpc":"2.0","method":"eth_coinbase","params":[]})");
}

TEST(EthRequestUnitTest, eth_mining) {
  ASSERT_EQ(eth_mining(),
            R"({"id":1,"jsonrpc":"2.0","method":"eth_mining","params":[]})");
}

TEST(EthRequestUnitTest, eth_hashrate) {
  ASSERT_EQ(eth_hashrate(),
            R"({"id":1,"jsonrpc":"2.0","method":"eth_hashrate","params":[]})");
}

TEST(EthRequestUnitTest, eth_gasPrice) {
  ASSERT_EQ(eth_gasPrice(),
            R"({"id":1,"jsonrpc":"2.0","method":"eth_gasPrice","params":[]})");
}

TEST(EthRequestUnitTest, eth_accounts) {
  ASSERT_EQ(eth_accounts(),
            R"({"id":1,"jsonrpc":"2.0","method":"eth_accounts","params":[]})");
}

TEST(EthRequestUnitTest, eth_blockNumber) {
  ASSERT_EQ(
      eth_blockNumber(),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_blockNumber","params":[]})");
}

TEST(EthRequestUnitTest, eth_getBalance) {
  ASSERT_EQ(
      eth_getBalance("0x407d73d8a49eeb85d32cf465507dd71d507100c1", "latest"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getBalance","params":["0x407d73d8a49eeb85d32cf465507dd71d507100c1","latest"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getStorageAt) {
  ASSERT_EQ(
      eth_getStorageAt("0x295a70b2de5e3953354a6a8344e616ed314d7251", "0x0",
                       "latest"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getStorageAt","params":["0x295a70b2de5e3953354a6a8344e616ed314d7251","0x0","latest"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getTransactionCount) {
  ASSERT_EQ(
      eth_getTransactionCount("0x407d73d8a49eeb85d32cf465507dd71d507100c1",
                              "latest"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getTransactionCount","params":["0x407d73d8a49eeb85d32cf465507dd71d507100c1","latest"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getBlockTransactionCountByHash) {
  ASSERT_EQ(
      eth_getBlockTransactionCountByHash(
          "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getBlockTransactionCountByHash","params":["0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getBlockTransactionCountByNumber) {
  ASSERT_EQ(
      eth_getBlockTransactionCountByNumber("0xe8"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getBlockTransactionCountByNumber","params":["0xe8"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getUncleCountByBlockHash) {
  ASSERT_EQ(
      eth_getUncleCountByBlockHash(
          "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getUncleCountByBlockHash","params":["0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getCode) {
  ASSERT_EQ(
      eth_getCode("0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b", "0x2"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getCode","params":["0xa94f5374fce5edbc8e2a8697c15331677e6ebf0b","0x2"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_sign) {
  ASSERT_EQ(
      eth_sign("0x9b2055d370f73ec7d8a03e965129118dc8f5bf83", "0xdeadbeaf"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_sign","params":["0x9b2055d370f73ec7d8a03e965129118dc8f5bf83","0xdeadbeaf"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_signTransaction) {
  ASSERT_EQ(
      eth_signTransaction("0xb60e8dd61c5d32be8058bb8eb970870f07233155",
                          "0xd46e8dd67c5d32be8058bb8eb970870f07244567",
                          "0x76c0", "0x9184e72a000", "0x9184e72a",
                          "0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f0"
                          "72445675058bb8eb970870f072445675",
                          ""),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_signTransaction","params":[{"data":"0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675","from":"0xb60e8dd61c5d32be8058bb8eb970870f07233155","gas":"0x76c0","gasPrice":"0x9184e72a000","to":"0xd46e8dd67c5d32be8058bb8eb970870f07244567","value":"0x9184e72a"}]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_sendTransaction) {
  ASSERT_EQ(
      eth_sendTransaction("0xb60e8dd61c5d32be8058bb8eb970870f07233155",
                          "0xd46e8dd67c5d32be8058bb8eb970870f07244567",
                          "0x76c0", "0x9184e72a000", "0x9184e72a",
                          "0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f0"
                          "72445675058bb8eb970870f072445675",
                          ""),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_sendTransaction","params":[{"data":"0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675","from":"0xb60e8dd61c5d32be8058bb8eb970870f07233155","gas":"0x76c0","gasPrice":"0x9184e72a000","to":"0xd46e8dd67c5d32be8058bb8eb970870f07244567","value":"0x9184e72a"}]})");  // NOLINT
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
}

TEST(EthRequestUnitTest, eth_estimateGas) {
  ASSERT_EQ(
      eth_estimateGas("0xb60e8dd61c5d32be8058bb8eb970870f07233155",
                      "0xd46e8dd67c5d32be8058bb8eb970870f07244567", "0x76c0",
                      "0x9184e72a000", "0x9184e72a",
                      "0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f07244"
                      "5675058bb8eb970870f072445675",
                      "latest"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_estimateGas","params":[{"data":"0xd46e8dd67c5d32be8d46e8dd67c5d32be8058bb8eb970870f072445675058bb8eb970870f072445675","from":"0xb60e8dd61c5d32be8058bb8eb970870f07233155","gas":"0x76c0","gasPrice":"0x9184e72a000","to":"0xd46e8dd67c5d32be8058bb8eb970870f07244567","value":"0x9184e72a"},"latest"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getBlockByHash) {
  ASSERT_EQ(
      eth_getBlockByHash(
          "0xdc0818cf78f21a8e70579cb46a43643f78291264dda342ae31049421c82d21ae",
          false),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getBlockByHash","params":["0xdc0818cf78f21a8e70579cb46a43643f78291264dda342ae31049421c82d21ae",false]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getBlockByNumber) {
  ASSERT_EQ(
      eth_getBlockByNumber("0x1b4", true),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getBlockByNumber","params":["0x1b4",true]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getTransactionByHash) {
  ASSERT_EQ(
      eth_getTransactionByHash(
          "0x88df016429689c079f3b2f6ad39fa052532c56795b733da78a91ebe6a713944b"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getTransactionByHash","params":["0x88df016429689c079f3b2f6ad39fa052532c56795b733da78a91ebe6a713944b"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getTransactionByBlockHashAndIndex) {
  ASSERT_EQ(
      eth_getTransactionByBlockHashAndIndex(
          "0xe670ec64341771606e55d6b4ca35a1a6b75ee3d5145a99d05921026d1527331",
          "0x0"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getTransactionByBlockHashAndIndex","params":["0xe670ec64341771606e55d6b4ca35a1a6b75ee3d5145a99d05921026d1527331","0x0"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getTransactionByBlockNumberAndIndex) {
  ASSERT_EQ(
      eth_getTransactionByBlockNumberAndIndex("0x29c", "0x0"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getTransactionByBlockNumberAndIndex","params":["0x29c","0x0"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getTransactionReceipt) {
  ASSERT_EQ(
      eth_getTransactionReceipt(
          "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getTransactionReceipt","params":["0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getUncleByBlockHashAndIndex) {
  ASSERT_EQ(
      eth_getUncleByBlockHashAndIndex(
          "0xc6ef2fc5426d6ad6fd9e2a26abeab0aa2411b7ab17f30a99d3cb96aed1d1055b",
          "0x0"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getUncleByBlockHashAndIndex","params":["0xc6ef2fc5426d6ad6fd9e2a26abeab0aa2411b7ab17f30a99d3cb96aed1d1055b","0x0"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getUncleByBlockNumberAndIndex) {
  ASSERT_EQ(
      eth_getUncleByBlockNumberAndIndex("0x29c", "0x0"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getUncleByBlockNumberAndIndex","params":["0x29c","0x0"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_getCompilers) {
  ASSERT_EQ(
      eth_getCompilers(),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getCompilers","params":[]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_compileSolidity) {
  ASSERT_EQ(
      eth_compileSolidity("contract test { function multiply(uint a) "
                          "returns(uint d) {   return a * 7;   } }"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_compileSolidity","params":["contract test { function multiply(uint a) returns(uint d) {   return a * 7;   } }"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_compileLLL) {
  ASSERT_EQ(
      eth_compileLLL("(returnlll (suicide (caller))) "),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_compileLLL","params":["(returnlll (suicide (caller))) "]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_compileSerpent) {
  ASSERT_EQ(
      eth_compileSerpent("/* some serpent */"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_compileSerpent","params":["/* some serpent */"]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_newFilter) {
  base::Value topics(base::Value::Type::LIST);
  topics.Append(base::Value(
      "0x000000000000000000000000a94f5374fce5edbc8e2a8697c15331677e6ebf0b"));
  base::Value sub_topics(base::Value::Type::LIST);
  sub_topics.Append(base::Value(
      "0x000000000000000000000000a94f5374fce5edbc8e2a8697c15331677e6ebf0b"));
  sub_topics.Append(base::Value(
      "0x0000000000000000000000000aff3454fce5edbc8cca8697c15331677e6ebccc"));
  topics.Append(std::move(sub_topics));
  ASSERT_EQ(
      eth_newFilter("0x1", "0x2", "0x8888f1f195afa192cfee860698584c030f4c9db1",
                    &topics),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_newFilter","params":[{"address":"0x8888f1f195afa192cfee860698584c030f4c9db1","fromBlock":"0x1","toBlock":"0x2","topics":["0x000000000000000000000000a94f5374fce5edbc8e2a8697c15331677e6ebf0b",["0x000000000000000000000000a94f5374fce5edbc8e2a8697c15331677e6ebf0b","0x0000000000000000000000000aff3454fce5edbc8cca8697c15331677e6ebccc"]]}]})");  // NOLINT
}

TEST(EthRequestUnitTest, eth_newBlockFilter) {
  ASSERT_EQ(
      eth_newBlockFilter(),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_newBlockFilter","params":[]})");
}

TEST(EthRequestUnitTest, eth_newPendingTransactionFilter) {
  ASSERT_EQ(
      eth_newPendingTransactionFilter(),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_newPendingTransactionFilter","params":[]})");
}

TEST(EthRequestUnitTest, eth_uninstallFilter) {
  ASSERT_EQ(
      eth_uninstallFilter("0xb"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_uninstallFilter","params":["0xb"]})");
}

TEST(EthRequestUnitTest, eth_getFilterChanges) {
  ASSERT_EQ(
      eth_getFilterChanges("0x16"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getFilterChanges","params":["0x16"]})");
}

TEST(EthRequestUnitTest, eth_getFilterLogs) {
  ASSERT_EQ(
      eth_getFilterLogs("0x16"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getFilterLogs","params":["0x16"]})");
}

TEST(EthRequestUnitTest, eth_getLogs) {
  base::Value topics(base::Value::Type::LIST);
  topics.Append(base::Value(
      "0x000000000000000000000000a94f5374fce5edbc8e2a8697c15331677e6ebf0b"));
  base::Value sub_topics(base::Value::Type::LIST);
  sub_topics.Append(base::Value(
      "0x000000000000000000000000a94f5374fce5edbc8e2a8697c15331677e6ebf0b"));
  sub_topics.Append(base::Value(
      "0x0000000000000000000000000aff3454fce5edbc8cca8697c15331677e6ebccc"));
  topics.Append(std::move(sub_topics));
  ASSERT_EQ(
      eth_getLogs(
          "0x1", "0x2", "0x8888f1f195afa192cfee860698584c030f4c9db1", &topics,
          "0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238"),
      R"({"id":1,"jsonrpc":"2.0","method":"eth_getLogs","params":[{"address":"0x8888f1f195afa192cfee860698584c030f4c9db1","blockhash":"0xb903239f8543d04b5dc1ba6579132b143087c68db1b2168786408fcbce568238","fromBlock":"0x1","toBlock":"0x2","topics":["0x000000000000000000000000a94f5374fce5edbc8e2a8697c15331677e6ebf0b",["0x000000000000000000000000a94f5374fce5edbc8e2a8697c15331677e6ebf0b","0x0000000000000000000000000aff3454fce5edbc8cca8697c15331677e6ebccc"]]}]})");  // NOLINT
}

}  // namespace brave_wallet
