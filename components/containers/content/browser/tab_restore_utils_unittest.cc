/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/containers/content/browser/tab_restore_utils.h"

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

class ContainersTabRestoreUtilsTest : public testing::Test {
 public:
  ContainersTabRestoreUtilsTest() {
    feature_list_.InitAndEnableFeature(features::kContainers);
  }

 protected:
  sessions::SerializedNavigationEntry CreateNavigation(
      const std::string& partition_domain,
      const std::string& partition_name) {
    sessions::SerializedNavigationEntry entry;
    entry.set_index(0);
    entry.set_virtual_url(GURL("https://example.com"));
    entry.set_storage_partition_key({partition_domain, partition_name});
    return entry;
  }

  sessions::SerializedNavigationEntry CreateNavigationWithoutPartition() {
    sessions::SerializedNavigationEntry entry;
    entry.set_index(0);
    entry.set_virtual_url(GURL("https://example.com"));
    return entry;
  }

  content::BrowserTaskEnvironment task_environment_;
  content::TestBrowserContext browser_context_;
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(ContainersTabRestoreUtilsTest, ValidContainerNavigation) {
  std::vector<sessions::SerializedNavigationEntry> navigations;
  navigations.push_back(
      CreateNavigation(kContainersStoragePartitionDomain,
                       "550e8400-e29b-41d4-a716-446655440000"));

  auto result =
      GetStoragePartitionConfigToRestore(&browser_context_, navigations, 0);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->partition_domain(), kContainersStoragePartitionDomain);
  EXPECT_EQ(result->partition_name(), "550e8400-e29b-41d4-a716-446655440000");
}

TEST_F(ContainersTabRestoreUtilsTest, NonContainerNavigation) {
  std::vector<sessions::SerializedNavigationEntry> navigations;
  navigations.push_back(CreateNavigationWithoutPartition());

  auto result =
      GetStoragePartitionConfigToRestore(&browser_context_, navigations, 0);

  EXPECT_FALSE(result.has_value());
}

TEST_F(ContainersTabRestoreUtilsTest, WrongPartitionDomain) {
  std::vector<sessions::SerializedNavigationEntry> navigations;
  navigations.push_back(CreateNavigation("extensions", "some-extension-id"));

  auto result =
      GetStoragePartitionConfigToRestore(&browser_context_, navigations, 0);

  EXPECT_FALSE(result.has_value());
}

TEST_F(ContainersTabRestoreUtilsTest, EmptyNavigations) {
  std::vector<sessions::SerializedNavigationEntry> navigations;

  auto result =
      GetStoragePartitionConfigToRestore(&browser_context_, navigations, 0);

  EXPECT_FALSE(result.has_value());
}

TEST_F(ContainersTabRestoreUtilsTest, NegativeSelectedNavigation) {
  std::vector<sessions::SerializedNavigationEntry> navigations;
  navigations.push_back(
      CreateNavigation(kContainersStoragePartitionDomain,
                       "550e8400-e29b-41d4-a716-446655440000"));

  auto result =
      GetStoragePartitionConfigToRestore(&browser_context_, navigations, -1);

  EXPECT_FALSE(result.has_value());
}

TEST_F(ContainersTabRestoreUtilsTest, OutOfRangeSelectedNavigation) {
  std::vector<sessions::SerializedNavigationEntry> navigations;
  navigations.push_back(
      CreateNavigation(kContainersStoragePartitionDomain,
                       "550e8400-e29b-41d4-a716-446655440000"));

  auto result =
      GetStoragePartitionConfigToRestore(&browser_context_, navigations, 1);

  EXPECT_FALSE(result.has_value());
}

TEST_F(ContainersTabRestoreUtilsTest, SelectsCorrectNavigation) {
  std::vector<sessions::SerializedNavigationEntry> navigations;
  navigations.push_back(CreateNavigationWithoutPartition());
  navigations.push_back(
      CreateNavigation(kContainersStoragePartitionDomain,
                       "550e8400-e29b-41d4-a716-446655440000"));
  navigations.push_back(CreateNavigationWithoutPartition());

  // Selecting index 0 (no partition) should return nullopt.
  EXPECT_FALSE(
      GetStoragePartitionConfigToRestore(&browser_context_, navigations, 0)
          .has_value());

  // Selecting index 1 (container partition) should return the config.
  auto result =
      GetStoragePartitionConfigToRestore(&browser_context_, navigations, 1);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->partition_domain(), kContainersStoragePartitionDomain);
  EXPECT_EQ(result->partition_name(), "550e8400-e29b-41d4-a716-446655440000");

  // Selecting index 2 (no partition) should return nullopt.
  EXPECT_FALSE(
      GetStoragePartitionConfigToRestore(&browser_context_, navigations, 2)
          .has_value());
}

TEST_F(ContainersTabRestoreUtilsTest, OffTheRecordContext) {
  content::TestBrowserContext otr_context;
  otr_context.set_is_off_the_record(true);

  std::vector<sessions::SerializedNavigationEntry> navigations;
  navigations.push_back(
      CreateNavigation(kContainersStoragePartitionDomain,
                       "550e8400-e29b-41d4-a716-446655440000"));

  auto result =
      GetStoragePartitionConfigToRestore(&otr_context, navigations, 0);

  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->partition_domain(), kContainersStoragePartitionDomain);
  EXPECT_EQ(result->partition_name(), "550e8400-e29b-41d4-a716-446655440000");
  EXPECT_TRUE(result->in_memory());
}

class ContainersTabRestoreUtilsFeatureDisabledTest
    : public ContainersTabRestoreUtilsTest {
 public:
  ContainersTabRestoreUtilsFeatureDisabledTest() {
    feature_list_.InitAndDisableFeature(features::kContainers);
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(ContainersTabRestoreUtilsFeatureDisabledTest,
       ReturnsNulloptWhenDisabled) {
  std::vector<sessions::SerializedNavigationEntry> navigations;
  navigations.push_back(CreateNavigationWithoutPartition());

  auto result =
      GetStoragePartitionConfigToRestore(&browser_context_, navigations, 0);

  EXPECT_FALSE(result.has_value());
}

TEST_F(ContainersTabRestoreUtilsFeatureDisabledTest,
       ReturnsNulloptWhenDisabledWithContainerKey) {
  std::vector<sessions::SerializedNavigationEntry> navigations;
  navigations.push_back(
      CreateNavigation(kContainersStoragePartitionDomain,
                       "550e8400-e29b-41d4-a716-446655440000"));

  auto result =
      GetStoragePartitionConfigToRestore(&browser_context_, navigations, 0);

  EXPECT_FALSE(result.has_value());
}

}  // namespace containers
