// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/core/browser/storage_partition_utils.h"

#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

using ContainersStoragePartitionUtilsTest = testing::Test;

TEST_F(ContainersStoragePartitionUtilsTest, IsContainerStoragePartitionDomain) {
  EXPECT_TRUE(IsContainerStoragePartitionDomain("container-abc"));
  EXPECT_TRUE(IsContainerStoragePartitionDomain("container-123"));
  EXPECT_TRUE(IsContainerStoragePartitionDomain("container-"));
  EXPECT_FALSE(IsContainerStoragePartitionDomain("notcontainer-abc"));
  EXPECT_FALSE(IsContainerStoragePartitionDomain("abc-container-"));
  EXPECT_FALSE(IsContainerStoragePartitionDomain(""));
}

TEST_F(ContainersStoragePartitionUtilsTest,
       GetContainerIdFromStoragePartitionDomain) {
  EXPECT_EQ(GetContainerIdFromStoragePartitionDomain("container-abc"),
            std::optional<std::string_view>("abc"));
  EXPECT_EQ(GetContainerIdFromStoragePartitionDomain("container-123"),
            std::optional<std::string_view>("123"));
  EXPECT_EQ(GetContainerIdFromStoragePartitionDomain("container-"),
            std::optional<std::string_view>(""));
  EXPECT_EQ(GetContainerIdFromStoragePartitionDomain("notcontainer-abc"),
            std::nullopt);
  EXPECT_EQ(GetContainerIdFromStoragePartitionDomain(""), std::nullopt);
}

TEST_F(ContainersStoragePartitionUtilsTest,
       GetContainerStoragePartitionDomain) {
  auto container = mojom::Container::New("testid", "Test Container");
  EXPECT_EQ(GetContainerStoragePartitionDomain(container), "container-testid");

  auto container2 = mojom::Container::New("", "EmptyId");
  EXPECT_EQ(GetContainerStoragePartitionDomain(container2), "container-");
}

}  // namespace containers
