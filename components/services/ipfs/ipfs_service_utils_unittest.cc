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
      "APIPort", "SwarmPort", "StorageSize");

  std::string expect =
      "{\"Addresses\":{\"API\":\"/ip4/127.0.0.1/tcp/APIPort\","
      "\"Gateway\":\"/ip4/127.0.0.1/tcp/GatewayPort\",\"Swarm\":"
      "[\"/ip4/0.0.0.0/tcp/SwarmPort\",\"/ip6/::/tcp/SwarmPort\""
      "]},\"Datastore\":{\"GCPeriod\":\"1h\",\"StorageMax\":"
      "\"StorageSize\"},\"Swarm\":{\"ConnMgr\":{\"HighWater\""
      ":300,\"LowWater\":50}}}";
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

}  // namespace ipfs
