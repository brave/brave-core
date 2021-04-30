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

TEST_F(IPFSJSONParserTest, GetGarbageCollectionFromJSON) {
  std::string error;
  ASSERT_TRUE(IPFSJSONParser::GetGarbageCollectionFromJSON(R"({
      "Error": "{error}",
      "Key": {
        "/": "{cid}"
      }
    })",
                                                           &error));

  ASSERT_EQ(error, "{error}");
  error.erase();
  ASSERT_TRUE(IPFSJSONParser::GetGarbageCollectionFromJSON(R"({
      "Key": {
        "/": "{cid}"
      }
    })",
                                                           &error));
  ASSERT_EQ(error, "");
  error.erase();
  ASSERT_FALSE(IPFSJSONParser::GetGarbageCollectionFromJSON(R"()", &error));
  ASSERT_TRUE(error.empty());
}

TEST_F(IPFSJSONParserTest, GetImportResponseFromJSON) {
  ipfs::ImportedData success;
  ASSERT_TRUE(IPFSJSONParser::GetImportResponseFromJSON(R"({
    "Name":"brave.com",
    "Hash":"QmYbK4SLaSvTKKAKvNZMwyzYPy4P3GqBPN6CZzbS73FxxU",
    "Size":"567857"
    })",
                                                        &success));
  ASSERT_EQ(success.hash, "QmYbK4SLaSvTKKAKvNZMwyzYPy4P3GqBPN6CZzbS73FxxU");
  // EXPECT_EQ(success.name, "brave.com");
  ASSERT_EQ(success.size, 567857);

  ipfs::ImportedData failed;
  ASSERT_TRUE(IPFSJSONParser::GetImportResponseFromJSON(R"({
    "Name":"brave.com",
    "Hash":"",
    "Size":"-1"
    })",
                                                        &failed));
  EXPECT_EQ(failed.hash, "");
  // EXPECT_EQ(failed.name, "brave.com");
  ASSERT_EQ(failed.size, -1);

  ipfs::ImportedData failed2;
  ASSERT_FALSE(IPFSJSONParser::GetImportResponseFromJSON(R"()", &failed2));
  EXPECT_EQ(failed2.hash, "");
  // EXPECT_EQ(failed2.name, "");
  ASSERT_EQ(failed2.size, -1);
}

TEST_F(IPFSJSONParserTest, GetParseKeysFromJSON) {
  std::unordered_map<std::string, std::string> parsed_keys;
  std::string response = R"({"Keys" : [)"
                         R"({"Name":"self","Id":"k51q...wal"},)"
                         R"({"Name":"MyCustomKey","Id":"k51q...wa1"}]})";
  ASSERT_TRUE(IPFSJSONParser::GetParseKeysFromJSON(response, &parsed_keys));
  ASSERT_EQ(parsed_keys.size(), size_t(2));
  ASSERT_TRUE(parsed_keys.count("self"));
  ASSERT_TRUE(parsed_keys.count("MyCustomKey"));
  EXPECT_EQ(parsed_keys["self"], "k51q...wal");
  EXPECT_EQ(parsed_keys["MyCustomKey"], "k51q...wa1");

  ASSERT_FALSE(IPFSJSONParser::GetParseKeysFromJSON("{}", &parsed_keys));
  ASSERT_EQ(parsed_keys.size(), size_t(2));
}

TEST_F(IPFSJSONParserTest, GetParseSingleKeyFromJSON) {
  std::string name;
  std::string value;
  std::string response = R"({"Name":"self","Id":"k51q...wal"})";
  ASSERT_TRUE(
      IPFSJSONParser::GetParseSingleKeyFromJSON(response, &name, &value));
  EXPECT_EQ(name, "self");
  EXPECT_EQ(value, "k51q...wal");
}
