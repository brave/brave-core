/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "brave/components/ipfs/ipfs_json_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

typedef testing::Test IPFSJSONParserTest;

TEST_F(IPFSJSONParserTest, GetPeersFromJSON) {
  std::vector<std::string> peers;
  ASSERT_TRUE(IPFSJSONParser::GetPeersFromJSON(R"(
      {
        "Peers": [
          {
            "Addr": "/ip4/10.8.0.206/tcp/4001",
            "Direction": "0",
            "Latency": "",
            "Muxer": "",
            "Peer": "QmaNcj4BMFQgE884rZSMqWEcqquWuv8QALzhpvPeHZGddd"
          },
          {
            "Addr": "/ip4/10.8.0.207/tcp/4001",
            "Direction": "0",
            "Latency": "",
            "Muxer": "",
            "Peer": "QmaNcj4BMFQgE884rZSMqWEcqquWuv8QALzhpvPeHZGeee"
          }
        ]
      })",
                                               &peers));

  ASSERT_EQ(peers.size(), uint64_t(2));
  ASSERT_EQ(peers[0],
            "/ip4/10.8.0.206/tcp/4001/p2p/"
            "QmaNcj4BMFQgE884rZSMqWEcqquWuv8QALzhpvPeHZGddd");  // NOLINT
  ASSERT_EQ(peers[1],
            "/ip4/10.8.0.207/tcp/4001/p2p/"
            "QmaNcj4BMFQgE884rZSMqWEcqquWuv8QALzhpvPeHZGeee");  // NOLINT
}

TEST_F(IPFSJSONParserTest, GetAddressesConfigFromJSON) {
  ipfs::AddressesConfig config;
  ASSERT_TRUE(IPFSJSONParser::GetAddressesConfigFromJSON(R"({
      "Key": "Addresses",
      "Value":
        {
          "API": "/ip4/127.0.0.1/tcp/5001",
          "Announce": [],
          "Gateway": "/ip4/127.0.0.1/tcp/8080",
          "NoAnnounce": [],
          "Swarm": [
            "/ip4/0.0.0.0/tcp/4001",
            "/ip6/::/tcp/4001",
            "/ip4/0.0.0.0/udp/4001/quic",
            "/ip6/::/udp/4001/quic"
          ]
        }
      })",
                                                         &config));

  ASSERT_EQ(config.api, "/ip4/127.0.0.1/tcp/5001");
  ASSERT_EQ(config.gateway, "/ip4/127.0.0.1/tcp/8080");
  ASSERT_EQ(config.swarm,
            (std::vector<std::string>{
                "/ip4/0.0.0.0/tcp/4001", "/ip6/::/tcp/4001",
                "/ip4/0.0.0.0/udp/4001/quic", "/ip6/::/udp/4001/quic"}));
}
