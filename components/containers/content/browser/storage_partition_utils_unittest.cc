// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/storage_partition_utils.h"

#include "base/types/optional_ref.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/test/storage_partition_test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

class StoragePartitionUtilsTest : public testing::Test {
 protected:
  StoragePartitionUtilsTest() = default;
  ~StoragePartitionUtilsTest() override = default;
};

TEST_F(StoragePartitionUtilsTest, IsContainersStoragePartition_ValidConfig) {
  auto config = content::CreateStoragePartitionConfigForTesting(
      /*in_memory=*/false,
      /*partition_domain=*/kContainersStoragePartitionDomain,
      /*partition_name=*/"test_container");

  EXPECT_TRUE(IsContainersStoragePartition(config));
}

TEST_F(StoragePartitionUtilsTest, IsContainersStoragePartition_WrongDomain) {
  auto config = content::CreateStoragePartitionConfigForTesting(
      /*in_memory=*/false,
      /*partition_domain=*/"wrong_domain",
      /*partition_name=*/"test_container");

  EXPECT_FALSE(IsContainersStoragePartition(config));
}

TEST_F(StoragePartitionUtilsTest,
       IsContainersStoragePartition_EmptyPartitionName) {
  auto config = content::CreateStoragePartitionConfigForTesting(
      /*in_memory=*/false,
      /*partition_domain=*/kContainersStoragePartitionDomain,
      /*partition_name=*/"");

  EXPECT_FALSE(IsContainersStoragePartition(config));
}

TEST_F(StoragePartitionUtilsTest, MaybeInheritStoragePartition_ValidConfig) {
  auto config = content::CreateStoragePartitionConfigForTesting(
      /*in_memory=*/false,
      /*partition_domain=*/kContainersStoragePartitionDomain,
      /*partition_name=*/"test_container");

  auto result = MaybeInheritStoragePartition(config);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), config);
}

TEST_F(StoragePartitionUtilsTest, MaybeInheritStoragePartition_NullOpt) {
  base::optional_ref<const content::StoragePartitionConfig> null_ref;

  auto result = MaybeInheritStoragePartition(null_ref);

  EXPECT_FALSE(result.has_value());
}

TEST_F(StoragePartitionUtilsTest, MaybeInheritStoragePartition_InvalidConfig) {
  auto config = content::CreateStoragePartitionConfigForTesting(
      /*in_memory=*/false,
      /*partition_domain=*/"wrong_domain",
      /*partition_name=*/"test_container");

  auto result = MaybeInheritStoragePartition(config);

  EXPECT_FALSE(result.has_value());
}

}  // namespace containers
