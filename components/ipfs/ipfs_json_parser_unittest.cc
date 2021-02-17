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
          "API": "/ip4/127.0.0.1/tcp/45001",
          "Announce": [],
          "Gateway": "/ip4/127.0.0.1/tcp/48080",
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

  ASSERT_EQ(config.api, "/ip4/127.0.0.1/tcp/45001");
  ASSERT_EQ(config.gateway, "/ip4/127.0.0.1/tcp/48080");
  ASSERT_EQ(config.swarm,
            (std::vector<std::string>{
                "/ip4/0.0.0.0/tcp/4001", "/ip6/::/tcp/4001",
                "/ip4/0.0.0.0/udp/4001/quic", "/ip6/::/udp/4001/quic"}));
}

TEST_F(IPFSJSONParserTest, GetRepoStatsFromJSON) {
  ipfs::RepoStats stat;

  ASSERT_EQ(stat.objects, uint64_t(0));
  ASSERT_EQ(stat.size, uint64_t(0));
  ASSERT_EQ(stat.storage_max, uint64_t(0));

  ASSERT_TRUE(IPFSJSONParser::GetRepoStatsFromJSON(R"({
        "NumObjects": 113,
        "RepoPath": "/some/path/to/repo",
        "RepoSize": 123456789,
        "StorageMax": 90000000,
        "Version": "fs-repo@10"
      })",
                                                   &stat));

  ASSERT_EQ(stat.objects, uint64_t(113));
  ASSERT_EQ(stat.size, uint64_t(123456789));
  ASSERT_EQ(stat.storage_max, uint64_t(90000000));
  ASSERT_EQ(stat.path, "/some/path/to/repo");
  ASSERT_EQ(stat.version, "fs-repo@10");
}

TEST_F(IPFSJSONParserTest, GetNodeInfoFromJSON) {
  ipfs::NodeInfo info;

  ASSERT_TRUE(IPFSJSONParser::GetNodeInfoFromJSON(R"({
      "Addresses": ["111.111.111.111"],
      "AgentVersion": "1.2.3.4",
      "ID": "idididid",
      "ProtocolVersion": "5.6.7.8",
      "Protocols": ["one", "two"],
      "PublicKey": "public_key"
    })",
                                                  &info));

  ASSERT_EQ(info.id, "idididid");
  ASSERT_EQ(info.version, "1.2.3.4");
}
