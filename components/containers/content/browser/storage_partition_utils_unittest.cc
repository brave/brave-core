// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/storage_partition_utils.h"

#include "base/test/scoped_feature_list.h"
#include "base/types/optional_ref.h"
#include "brave/components/containers/core/common/features.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/test/storage_partition_test_helpers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

class ContainersStoragePartitionUtilsTest : public testing::Test {
 public:
  ContainersStoragePartitionUtilsTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kContainers);
  }
  ~ContainersStoragePartitionUtilsTest() override = default;

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(ContainersStoragePartitionUtilsTest, IsContainersStoragePartition) {
  struct {
    std::string partition_domain;
    std::string partition_name;
    bool expected_result;
  } const test_cases[] = {
      {kContainersStoragePartitionDomain, "test-container", true},
      {kContainersStoragePartitionDomain, "123-test-container", true},
      {"", "", false},
      {kContainersStoragePartitionDomain, "", false},
      {"wrong-domain", "test-container", false},
      {kContainersStoragePartitionDomain, "test_container", false},
      {kContainersStoragePartitionDomain, "test container", false},
      {kContainersStoragePartitionDomain, "test_container-", false},
      {kContainersStoragePartitionDomain, "-test_container", false},
      {kContainersStoragePartitionDomain, "123-test_container", false},
  };

  for (const bool in_memory : {false, true}) {
    for (const auto& test_case : test_cases) {
      SCOPED_TRACE(test_case.partition_domain + "+" + test_case.partition_name);
      auto config = content::CreateStoragePartitionConfigForTesting(
          in_memory, test_case.partition_domain, test_case.partition_name);
      EXPECT_EQ(IsContainersStoragePartition(config),
                test_case.expected_result);
    }
  }
}

TEST_F(ContainersStoragePartitionUtilsTest, IsContainersStoragePartitionKey) {
  struct {
    std::string partition_domain;
    std::string partition_name;
    bool expected_result;
  } const test_cases[] = {
      {kContainersStoragePartitionDomain, "test-container", true},
      {kContainersStoragePartitionDomain, "123-test-container", true},
      {"", "", false},
      {kContainersStoragePartitionDomain, "", false},
      {"wrong-domain", "test-container", false},
      {kContainersStoragePartitionDomain, "test_container", false},
      {kContainersStoragePartitionDomain, "test container", false},
      {kContainersStoragePartitionDomain, "test_container-", false},
      {kContainersStoragePartitionDomain, "-test_container", false},
      {kContainersStoragePartitionDomain, "123-test_container", false},
  };

  for (const auto& test_case : test_cases) {
    SCOPED_TRACE(test_case.partition_domain + "+" + test_case.partition_name);
    EXPECT_EQ(IsContainersStoragePartitionKey(test_case.partition_domain,
                                              test_case.partition_name),
              test_case.expected_result);
  }
}

TEST_F(ContainersStoragePartitionUtilsTest,
       IsValidStoragePartitionKeyComponent) {
  struct {
    std::string partition_name;
    bool expected_result;
  } const test_cases[] = {
      {"container", true},           {"test-container", true},
      {"123-test-container", true},  {"", false},
      {"test_container", false},     {"test container", false},
      {"test_container-", false},    {"-test_container", false},
      {"123-test_container", false},
  };

  for (const auto& test_case : test_cases) {
    SCOPED_TRACE(test_case.partition_name);
    EXPECT_EQ(IsValidStoragePartitionKeyComponent(test_case.partition_name),
              test_case.expected_result);
  }
}

TEST_F(ContainersStoragePartitionUtilsTest,
       MaybeInheritStoragePartition_ValidConfig) {
  auto config = content::CreateStoragePartitionConfigForTesting(
      /*in_memory=*/false,
      /*partition_domain=*/kContainersStoragePartitionDomain,
      /*partition_name=*/"test-container");

  auto result = MaybeInheritStoragePartition(config);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), config);
}

TEST_F(ContainersStoragePartitionUtilsTest,
       MaybeInheritStoragePartition_NullOpt) {
  base::optional_ref<const content::StoragePartitionConfig> null_ref;

  auto result = MaybeInheritStoragePartition(null_ref);

  EXPECT_FALSE(result.has_value());
}

TEST_F(ContainersStoragePartitionUtilsTest,
       MaybeInheritStoragePartition_InvalidConfig) {
  auto config = content::CreateStoragePartitionConfigForTesting(
      /*in_memory=*/false,
      /*partition_domain=*/"wrong-domain",
      /*partition_name=*/"test-container");

  auto result = MaybeInheritStoragePartition(config);

  EXPECT_FALSE(result.has_value());
}

}  // namespace containers
