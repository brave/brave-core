/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "base/json/json_reader.h"
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

TEST_F(IPFSJSONParserTest, GetPeersFromConfigJSONTest) {
  std::string json = R"({
      "Peering": {
        "Peers": [
            {
                "ID": "fdfsa"
            }
        ]
      }
    })";
  std::vector<std::string> peers;
  ASSERT_TRUE(IPFSJSONParser::GetPeersFromConfigJSON(json, &peers));
  EXPECT_EQ(peers.size(), size_t(1));
  EXPECT_EQ(peers[0], "fdfsa");
  json = R"({
      "Peering": {
        "Peers": [
            {
                "ID": "fdfsa",
                "Addrs": null
            }
        ]
      }
    })";
  peers.clear();
  ASSERT_TRUE(IPFSJSONParser::GetPeersFromConfigJSON(json, &peers));
  EXPECT_EQ(peers.size(), size_t(1));
  EXPECT_EQ(peers[0], "fdfsa");
  peers.clear();
  json = R"({
      "Peering": {
        "Peers": [
            {
                "ID1": "fdfsa"
            }
        ]
      }
    })";
  ASSERT_TRUE(IPFSJSONParser::GetPeersFromConfigJSON(json, &peers));
  ASSERT_TRUE(peers.empty());
  json = R"({
      "Peering": {
        "Peers": {}
      }
    })";
  ASSERT_FALSE(IPFSJSONParser::GetPeersFromConfigJSON(json, &peers));
  ASSERT_TRUE(peers.empty());
  json = R"({
      "Peering": {
        "Peers": null
      }
    })";
  ASSERT_FALSE(IPFSJSONParser::GetPeersFromConfigJSON(json, &peers));
  ASSERT_TRUE(peers.empty());
  json = R"({
    "Peering": {
        "Peers": [
            {
                "Addrs": null,
                "ID": "QmQCU2EcMqAqQPR2i9bChDtGNJchTbq5TbXJJ16u19uLTa"
            },
            {
                "Addrs": [],
                "ID": "QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ"
            },
            {
                "Addrs": ["/ip4/46.21.210.45/tcp/14406"],
                "ID": "QmcZf59bWwK5XFi76CZX8cbJ4BhTzzA3gU1ZjYZcYW3dwt"
            }
        ]
    }
  })";
  ASSERT_TRUE(IPFSJSONParser::GetPeersFromConfigJSON(json, &peers));
  EXPECT_EQ(peers.size(), size_t(3));
  EXPECT_EQ(peers[0], "QmQCU2EcMqAqQPR2i9bChDtGNJchTbq5TbXJJ16u19uLTa");
  EXPECT_EQ(peers[1], "QmaCpDMGvV2BGHeYERUEnRQAwe3N8SzbUtfsmvsqQLuvuJ");
  EXPECT_EQ(peers[2],
            "/ip4/46.21.210.45/tcp/14406/p2p/"
            "QmcZf59bWwK5XFi76CZX8cbJ4BhTzzA3gU1ZjYZcYW3dwt");
}

TEST_F(IPFSJSONParserTest, PutNewPeerToConfigJSONTest) {
  std::string json = R"({})";
  std::string new_peer = "QmNewPeer";
  std::string result = IPFSJSONParser::PutNewPeerToConfigJSON(json, new_peer);
  EXPECT_EQ(result, "{\"Peering\":{\"Peers\":[{\"ID\":\"QmNewPeer\"}]}}");

  json = R"({"Peering":{}})";
  result = IPFSJSONParser::PutNewPeerToConfigJSON(json, new_peer);
  EXPECT_EQ(result, "{\"Peering\":{\"Peers\":[{\"ID\":\"QmNewPeer\"}]}}");

  json = R"({"Peering":{"Peers":null }})";
  result = IPFSJSONParser::PutNewPeerToConfigJSON(json, new_peer);
  EXPECT_EQ(result, "{\"Peering\":{\"Peers\":[{\"ID\":\"QmNewPeer\"}]}}");

  json = R"({"Peering":{"Peers":[]}})";
  result = IPFSJSONParser::PutNewPeerToConfigJSON(json, new_peer);
  EXPECT_EQ(result, "{\"Peering\":{\"Peers\":[{\"ID\":\"QmNewPeer\"}]}}");

  json = R"({"Peering":{"Peers":[]}})";
  result = IPFSJSONParser::PutNewPeerToConfigJSON(json, "");
  EXPECT_EQ(result, "");

  json = R"({"Peering":{"Peers":[]}})";
  result = IPFSJSONParser::PutNewPeerToConfigJSON(json, "");
  EXPECT_EQ(result, "");

  json = R"({"Peering":{"Peers":[{"ID":"QmA"}]}})";
  result = IPFSJSONParser::PutNewPeerToConfigJSON(json, new_peer);
  EXPECT_EQ(result,
            "{\"Peering\":{\"Peers\":[{\"ID\":\"QmA\"},"
            "{\"ID\":\"QmNewPeer\"}]}}");

  json = R"({"Peering":{"Peers":null}})";
  new_peer = "/a/p2p/QmNewPeer";
  result = IPFSJSONParser::PutNewPeerToConfigJSON(json, new_peer);
  EXPECT_EQ(result,
            "{\"Peering\":{\"Peers\":[{\"Addrs\":[\"/a\"],"
            "\"ID\":\"QmNewPeer\"}]}}");

  json = R"({"Peering":{"Peers":[{"ID":"QmA"}]}})";
  new_peer = "/a/p2p/QmA";
  result = IPFSJSONParser::PutNewPeerToConfigJSON(json, new_peer);
  EXPECT_EQ(result,
            "{\"Peering\":{\"Peers\":[{\"Addrs\":[\"/a\"],"
            "\"ID\":\"QmA\"}]}}");

  json = R"({"Peering":{"Peers":[{"ID":"QmA","Addrs":["/a","/b"]}]}})";
  new_peer = "/a/p2p/QmA";
  result = IPFSJSONParser::PutNewPeerToConfigJSON(json, new_peer);
  EXPECT_EQ(result,
            "{\"Peering\":{\"Peers\":"
            "[{\"Addrs\":[\"/a\",\"/b\",\"/a\"],\"ID\":\"QmA\"}]}}");

  json = R"({"Peering":{"Peers":[{"ID":"QmA","Addrs":["/a","/b"]}]}})";
  new_peer = "/c/p2p/QmA";
  result = IPFSJSONParser::PutNewPeerToConfigJSON(json, new_peer);
  EXPECT_EQ(result,
            "{\"Peering\":{\"Peers\":"
            "[{\"Addrs\":[\"/a\",\"/b\",\"/c\"],\"ID\":\"QmA\"}]}}");

  json = R"({"Peering":{"Peers":[{"ID":"QmA","Addrs":["/a","/b"]}]}})";
  new_peer = "/a/p2p/QmB";
  result = IPFSJSONParser::PutNewPeerToConfigJSON(json, new_peer);
  EXPECT_EQ(result,
            "{\"Peering\":{\"Peers\":["
            "{\"Addrs\":[\"/a\",\"/b\"],\"ID\":\"QmA\"},"
            "{\"Addrs\":[\"/a\"],\"ID\":\"QmB\"}]}}");
}

TEST_F(IPFSJSONParserTest, RemovePeerFromConfigJSONTest) {
  std::string json = R"({})";
  std::string result =
      IPFSJSONParser::RemovePeerFromConfigJSON(json, "QmNewPeer", "");
  EXPECT_EQ(result, json);

  json = R"({{})";
  result = IPFSJSONParser::RemovePeerFromConfigJSON(json, "QmNewPeer", "");
  EXPECT_EQ(result, std::string());

  json = R"({"Peering":{"Peers":[{"ID":"QmA","Addrs":["/a","/b"]}]}})";
  result = IPFSJSONParser::RemovePeerFromConfigJSON(json, "QmNewPeer", "");
  EXPECT_EQ(result, json);

  json = R"({"Peering":{"Peers":[{"ID":"QmA","Addrs":["/a","/b"]}]}})";
  result = IPFSJSONParser::RemovePeerFromConfigJSON(json, "QmA", "");
  EXPECT_EQ(result, "{\"Peering\":{\"Peers\":[]}}");

  json = R"({"Peering":{"Peers":[{"ID":"QmA","Addrs":["/a","/b"]}]}})";
  result = IPFSJSONParser::RemovePeerFromConfigJSON(json, "QmA", "/ac");
  EXPECT_EQ(result, json);

  json = R"({"Peering":{"Peers":[{"ID":"QmA"}]}})";
  result = IPFSJSONParser::RemovePeerFromConfigJSON(json, "QmA", "/ac");
  EXPECT_EQ(result, json);

  json = R"({"Peering":{"Peers":[{"ID":"QmA"}]}})";
  result = IPFSJSONParser::RemovePeerFromConfigJSON(json, "QmA", "");
  EXPECT_EQ(result, "{\"Peering\":{\"Peers\":[]}}");

  json = R"({"Peering":{"Peers":[{"ID":"QmA","Addrs":["/a","/b"]}]}})";
  result = IPFSJSONParser::RemovePeerFromConfigJSON(json, "QmA", "/a");
  EXPECT_EQ(result,
            "{\"Peering\":{\"Peers\":"
            "[{\"Addrs\":[\"/b\"],\"ID\":\"QmA\"}]}}");
}
