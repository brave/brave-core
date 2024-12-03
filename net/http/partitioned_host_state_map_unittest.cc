/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/net/http/partitioned_host_state_map.h"

#include <map>
#include <string>
#include <string_view>

#include "base/containers/span.h"
#include "crypto/sha2.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace {

using HostHash = std::array<uint8_t, crypto::kSHA256Length>;
using PartitionedMap = PartitionedHostStateMap<std::map<HostHash, std::string>>;

HostHash HashHost(std::string_view canonicalized_host) {
  if (canonicalized_host.empty()) {
    return {};
  }
  return crypto::SHA256Hash(base::as_byte_span(canonicalized_host));
}

}  // namespace

TEST(PartitionedHostStateMapTest, WithoutPartitionHash) {
  PartitionedMap map;
  EXPECT_FALSE(map.HasPartitionHash());
  EXPECT_FALSE(map.IsPartitionHashValid());

  map[HashHost("key1")] = "1";
  map[HashHost("key2")] = "2";
  EXPECT_EQ(map.size(), 2u);

  EXPECT_EQ(map.find(HashHost("key1")), map.begin());
  EXPECT_EQ(map.find(HashHost("key1"))->second, "1");
  EXPECT_EQ(map.find(HashHost("key2"))->second, "2");
  EXPECT_EQ(map.find(HashHost("key3")), map.end());

  map.erase(HashHost("key1"));
  EXPECT_EQ(map.size(), 1u);
  map.erase(HashHost("key2"));
  EXPECT_EQ(map.size(), 0u);

  EXPECT_FALSE(map.DeleteDataInAllPartitions(HashHost("key1")));
}

TEST(PartitionedHostStateMapTest, InvalidPartitionHash) {
  PartitionedMap map;
  // Empty string is an invalid partition. It means it should not be persisted.
  auto auto_reset_partition_hash = map.SetScopedPartitionHash(HashHost(""));
  EXPECT_TRUE(map.HasPartitionHash());
  EXPECT_FALSE(map.IsPartitionHashValid());

  // Nothing should be persisted when an invalid hash is set.
  map[HashHost("key1")] = "1";
  map[HashHost("key2")] = "2";
  EXPECT_EQ(map.size(), 0u);

  EXPECT_EQ(map.find(HashHost("key1")), map.end());
  EXPECT_EQ(map.find(HashHost("key2")), map.end());
  EXPECT_EQ(map.find(HashHost("key3")), map.end());

  map.erase(HashHost("key1"));
  EXPECT_EQ(map.size(), 0u);
  map.erase(HashHost("key2"));
  EXPECT_EQ(map.size(), 0u);

  EXPECT_FALSE(map.DeleteDataInAllPartitions(HashHost("key1")));
}

TEST(PartitionedHostStateMapTest, ValidPartitionHash) {
  PartitionedMap map;
  auto auto_reset_partition_hash =
      map.SetScopedPartitionHash(HashHost("partition1"));
  EXPECT_TRUE(map.HasPartitionHash());
  EXPECT_TRUE(map.IsPartitionHashValid());

  map[HashHost("key1")] = "11";
  map[HashHost("key2")] = "12";
  EXPECT_EQ(map.size(), 2u);

  EXPECT_EQ(map.find(HashHost("key1"))->second, "11");
  EXPECT_EQ(map.find(HashHost("key2"))->second, "12");
  EXPECT_EQ(map.find(HashHost("key3")), map.end());

  map.erase(HashHost("key1"));
  EXPECT_EQ(map.size(), 1u);
  map.erase(HashHost("key2"));
  EXPECT_EQ(map.size(), 0u);

  EXPECT_FALSE(map.DeleteDataInAllPartitions(HashHost("key1")));
}

TEST(PartitionedHostStateMapTest, MultiplePartitions) {
  PartitionedMap map;
  auto auto_reset_partition_hash =
      map.SetScopedPartitionHash(HashHost("partition1"));

  map[HashHost("key1")] = "11";
  map[HashHost("key2")] = "12";
  EXPECT_EQ(map.size(), 2u);
  EXPECT_EQ(map.find(HashHost("key1"))->second, "11");
  EXPECT_EQ(map.find(HashHost("key2"))->second, "12");
  EXPECT_EQ(map.find(HashHost("key3")), map.end());

  auto_reset_partition_hash =
      map.SetScopedPartitionHash(HashHost("partition2"));
  map[HashHost("key1")] = "21";
  map[HashHost("key2")] = "22";
  EXPECT_EQ(map.size(), 4u);

  EXPECT_EQ(map.find(HashHost("key1"))->second, "21");
  EXPECT_EQ(map.find(HashHost("key2"))->second, "22");
  EXPECT_EQ(map.find(HashHost("key3")), map.end());

  EXPECT_EQ(map.erase(HashHost("key2")), 1u);
  EXPECT_EQ(map.size(), 3u);

  auto_reset_partition_hash =
      map.SetScopedPartitionHash(HashHost("partition3"));
  EXPECT_EQ(map.find(HashHost("key1")), map.end());
  EXPECT_EQ(map.find(HashHost("key3")), map.end());
  EXPECT_EQ(map.find(HashHost("key2")), map.end());

  EXPECT_EQ(map.erase(HashHost("key2")), 0u);
  EXPECT_EQ(map.size(), 3u);

  // Should delete key1 in partition1 and partition2.
  EXPECT_TRUE(map.DeleteDataInAllPartitions(HashHost("key1")));
  EXPECT_EQ(map.size(), 1u);

  auto_reset_partition_hash =
      map.SetScopedPartitionHash(HashHost("partition1"));
  EXPECT_EQ(map.find(HashHost("key1")), map.end());
  EXPECT_EQ(map.find(HashHost("key2"))->second, "12");
}

}  // namespace net
