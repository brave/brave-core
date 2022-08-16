/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"

#include "brave/components/services/ipfs/ipfs_service_utils.h"

#include "brave/components/services/ipfs/public/mojom/ipfs_service.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ipfs {

typedef testing::Test IPFSServiceUtils;

TEST_F(IPFSServiceUtils, UpdateConfigJSONTest) {
  std::string json = R"({})";
  std::string updated;
  auto config = ipfs::mojom::IpfsConfig::New(
      base::FilePath(), base::FilePath(), base::FilePath(), "GatewayPort",
      "APIPort", "SwarmPort", "StorageSize", absl::nullopt);

  std::string expect =
      "{\"Addresses\":{\"API\":\"/ip4/127.0.0.1/tcp/APIPort\","
      "\"Gateway\":\"/ip4/127.0.0.1/tcp/GatewayPort\",\"Swarm\":"
      "[\"/ip4/0.0.0.0/tcp/SwarmPort\",\"/ip6/::/tcp/SwarmPort\""
      "]},\"Datastore\":{\"GCPeriod\":\"1h\",\"StorageMax\":"
      "\"StorageSize\"},\"Swarm\":{\"ConnMgr\":{\"GracePeriod\":\"20s\","
      "\"HighWater\":40,\"LowWater\":20}}}";
  ASSERT_TRUE(UpdateConfigJSON(json, config.get(), &updated));
  EXPECT_EQ(updated, expect);
  updated.clear();
  json = R"({
    "Addresses":{
        "API":"/ip4/127.0.0.1/tcp/111",
        "Gateway":"/ip4/127.0.0.1/tcp/111",
        "Swarm":[
          "/ip4/0.0.0.0/tcp/222",
          "/ip6/::/tcp/222",
          "/ip6/::/udp/666"
        ]
    },
    "Datastore":{
        "GCPeriod":"6h",
        "StorageMax":"9GB"
    },
    "Swarm":{
        "ConnMgr":{
          "HighWater":0,
          "LowWater":0
        }
    }
  })";
  ASSERT_TRUE(UpdateConfigJSON(json, config.get(), &updated));
  EXPECT_EQ(updated, expect);
  updated.clear();
  ASSERT_FALSE(UpdateConfigJSON("{", config.get(), &updated));
  EXPECT_EQ(updated, "");
  updated.clear();
  ASSERT_FALSE(UpdateConfigJSON("[]", config.get(), &updated));
  EXPECT_EQ(updated, "");
}

TEST_F(IPFSServiceUtils, DNSResolversRemove) {
  std::string updated;
  {
    std::string json = R"({})";

    auto config = ipfs::mojom::IpfsConfig::New(
        base::FilePath(), base::FilePath(), base::FilePath(), "GatewayPort",
        "APIPort", "SwarmPort", "StorageSize",
        "https://cloudflare.com/dns-query");

    std::string expect =
        "{\"Addresses\":{\"API\":\"/ip4/127.0.0.1/tcp/APIPort\","
        "\"Gateway\":\"/ip4/127.0.0.1/tcp/GatewayPort\",\"Swarm\":"
        "[\"/ip4/0.0.0.0/tcp/SwarmPort\",\"/ip6/::/tcp/SwarmPort\""
        "]},\"DNS\":{\"Resolvers\":{\".\":\"https://cloudflare.com/"
        "dns-query\"}},"
        "\"Datastore\":{\"GCPeriod\":\"1h\",\"StorageMax\":"
        "\"StorageSize\"},\"Swarm\":{\"ConnMgr\":{\"GracePeriod\":\"20s\","
        "\"HighWater\":40,\"LowWater\":20}}}";

    EXPECT_TRUE(UpdateConfigJSON(json, config.get(), &updated));
    EXPECT_EQ(updated, expect);
  }

  std::string json = updated;

  auto config = ipfs::mojom::IpfsConfig::New(
      base::FilePath(), base::FilePath(), base::FilePath(), "GatewayPort",
      "APIPort", "SwarmPort", "StorageSize", absl::nullopt);

  std::string expect =
      "{\"Addresses\":{\"API\":\"/ip4/127.0.0.1/tcp/APIPort\","
      "\"Gateway\":\"/ip4/127.0.0.1/tcp/GatewayPort\",\"Swarm\":"
      "[\"/ip4/0.0.0.0/tcp/SwarmPort\",\"/ip6/::/tcp/SwarmPort\""
      "]},"
      "\"Datastore\":{\"GCPeriod\":\"1h\",\"StorageMax\":"
      "\"StorageSize\"},\"Swarm\":{\"ConnMgr\":{\"GracePeriod\":\"20s\","
      "\"HighWater\":40,\"LowWater\":20}}}";

  EXPECT_TRUE(UpdateConfigJSON(json, config.get(), &updated));
  ASSERT_EQ(updated, expect);
}

TEST_F(IPFSServiceUtils, DNSResolversUpdate) {
  std::string json = R"({})";
  std::string updated;
  auto config = ipfs::mojom::IpfsConfig::New(
      base::FilePath(), base::FilePath(), base::FilePath(), "GatewayPort",
      "APIPort", "SwarmPort", "StorageSize",
      "https://cloudflare.com/dns-query");

  std::string expect =
      "{\"Addresses\":{\"API\":\"/ip4/127.0.0.1/tcp/APIPort\","
      "\"Gateway\":\"/ip4/127.0.0.1/tcp/GatewayPort\",\"Swarm\":"
      "[\"/ip4/0.0.0.0/tcp/SwarmPort\",\"/ip6/::/tcp/SwarmPort\""
      "]},\"DNS\":{\"Resolvers\":{\".\":\"https://cloudflare.com/dns-query\"}},"
      "\"Datastore\":{\"GCPeriod\":\"1h\",\"StorageMax\":"
      "\"StorageSize\"},\"Swarm\":{\"ConnMgr\":{\"GracePeriod\":\"20s\","
      "\"HighWater\":40,\"LowWater\":20}}}";
  ASSERT_TRUE(UpdateConfigJSON(json, config.get(), &updated));
  EXPECT_EQ(updated, expect);
}

TEST_F(IPFSServiceUtils, DNSResolversUpdate_DnsHasRFC8484Template) {
  std::string json = R"({})";
  std::string updated;
  auto config = ipfs::mojom::IpfsConfig::New(
      base::FilePath(), base::FilePath(), base::FilePath(), "GatewayPort",
      "APIPort", "SwarmPort", "StorageSize",
      "https://cloudflare.com/dns-query{?dns}");

  std::string expect =
      "{\"Addresses\":{\"API\":\"/ip4/127.0.0.1/tcp/APIPort\","
      "\"Gateway\":\"/ip4/127.0.0.1/tcp/GatewayPort\",\"Swarm\":"
      "[\"/ip4/0.0.0.0/tcp/SwarmPort\",\"/ip6/::/tcp/SwarmPort\""
      "]},\"DNS\":{\"Resolvers\":{\".\":\"https://cloudflare.com/dns-query\"}},"
      "\"Datastore\":{\"GCPeriod\":\"1h\",\"StorageMax\":"
      "\"StorageSize\"},\"Swarm\":{\"ConnMgr\":{\"GracePeriod\":\"20s\","
      "\"HighWater\":40,\"LowWater\":20}}}";
  ASSERT_TRUE(UpdateConfigJSON(json, config.get(), &updated));
  EXPECT_EQ(updated, expect);
}

TEST_F(IPFSServiceUtils, GetVerisonFromNodeFilename) {
  EXPECT_EQ(
      ipfs::GetVersionFromNodeFilename("go-ipfs_v0.9.0-rc1_windows-amd64"),
      "0.9.0");
  EXPECT_EQ(
      ipfs::GetVersionFromNodeFilename("go-ipfs_v0.9.0-rc12_windows-amd64"),
      "0.9.0");
  EXPECT_EQ(ipfs::GetVersionFromNodeFilename("go-ipfs_v0.9.0_windows-amd64"),
            "0.9.0");

  EXPECT_EQ(
      ipfs::GetVersionFromNodeFilename("go-ipfs_v0.10.0-rc1_darwin-amd64"),
      "0.10.0");
  EXPECT_EQ(
      ipfs::GetVersionFromNodeFilename("go-ipfs_v0.10.0-rc12_darwin-amd64"),
      "0.10.0");
  EXPECT_EQ(ipfs::GetVersionFromNodeFilename("go-ipfs_v0.10.0_darwin-amd64"),
            "0.10.0");

  EXPECT_EQ(
      ipfs::GetVersionFromNodeFilename("go-ipfs_v0.10.0-rc1_darwin-arm64"),
      "0.10.0");
  EXPECT_EQ(
      ipfs::GetVersionFromNodeFilename("go-ipfs_v0.10.0-rc12_darwin-arm64"),
      "0.10.0");
  EXPECT_EQ(ipfs::GetVersionFromNodeFilename("go-ipfs_v0.10.0_darwin-arm64"),
            "0.10.0");

  EXPECT_EQ(ipfs::GetVersionFromNodeFilename("go-ipfs_v0.10.0-rc1_linux-amd64"),
            "0.10.0");
  EXPECT_EQ(
      ipfs::GetVersionFromNodeFilename("go-ipfs_v0.10.0-rc12_linux-amd64"),
      "0.10.0");
  EXPECT_EQ(ipfs::GetVersionFromNodeFilename("go-ipfs_v0.10.0_linux-amd64"),
            "0.10.0");

  EXPECT_EQ(ipfs::GetVersionFromNodeFilename("go-ipfs_v0.10.0_linux"), "");
  EXPECT_EQ(ipfs::GetVersionFromNodeFilename(""), "");
  EXPECT_EQ(ipfs::GetVersionFromNodeFilename("ipfs.exe"), "");
}

}  // namespace ipfs
