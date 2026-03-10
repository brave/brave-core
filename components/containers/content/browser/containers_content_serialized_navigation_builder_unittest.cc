/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/scoped_feature_list.h"
#include "brave/components/containers/content/browser/session_utils.h"
#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "components/sessions/content/content_serialized_navigation_builder.h"
#include "components/sessions/core/serialized_navigation_entry.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_entry_restore_context.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

namespace {

constexpr char kContainerId[] = "550e8400-e29b-41d4-a716-446655440000";

std::pair<std::string, std::string> MakeContainerStoragePartitionKey() {
  return {kContainersStoragePartitionDomain, kContainerId};
}

std::unique_ptr<content::NavigationEntry> MakeNavigationEntryForTest() {
  std::unique_ptr<content::NavigationEntry> entry(
      content::NavigationEntry::Create());
  entry->SetURL(GURL("https://example.com"));
  entry->SetVirtualURL(GURL("https://example.com"));
  return entry;
}

}  // namespace

class ContainersContentSerializedNavigationBuilderTest : public testing::Test {
 public:
  ContainersContentSerializedNavigationBuilderTest() {
    feature_list_.InitAndEnableFeature(features::kContainers);
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  content::TestBrowserContext browser_context_;
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(ContainersContentSerializedNavigationBuilderTest,
       FromNavigationEntryStoresContainerPartitionData) {
  auto entry = MakeNavigationEntryForTest();
  const auto storage_partition_key = MakeContainerStoragePartitionKey();
  entry->SetStoragePartitionKeyToRestore(storage_partition_key);

  sessions::SerializedNavigationEntry navigation =
      sessions::ContentSerializedNavigationBuilder::FromNavigationEntry(
          /*index=*/0, entry.get());

  auto expected_prefix = StoragePartitionKeyToUrlPrefix(storage_partition_key);
  ASSERT_TRUE(expected_prefix.has_value());
  EXPECT_EQ(navigation.virtual_url_prefix(), *expected_prefix);

  ASSERT_TRUE(navigation.storage_partition_key().has_value());
  EXPECT_EQ(*navigation.storage_partition_key(), storage_partition_key);
}

TEST_F(ContainersContentSerializedNavigationBuilderTest,
       FromNavigationEntryIgnoresNonContainerPartitionData) {
  auto entry = MakeNavigationEntryForTest();
  entry->SetStoragePartitionKeyToRestore({"extensions", "some-extension-id"});

  sessions::SerializedNavigationEntry navigation =
      sessions::ContentSerializedNavigationBuilder::FromNavigationEntry(
          /*index=*/0, entry.get());

  EXPECT_TRUE(navigation.virtual_url_prefix().empty());
  EXPECT_FALSE(navigation.storage_partition_key().has_value());
}

TEST_F(ContainersContentSerializedNavigationBuilderTest,
       ToNavigationEntryRestoresContainerPartitionData) {
  const auto storage_partition_key = MakeContainerStoragePartitionKey();
  sessions::SerializedNavigationEntry navigation;
  navigation.set_index(0);
  navigation.set_virtual_url(GURL("https://example.com"));
  navigation.set_storage_partition_key(storage_partition_key);

  std::unique_ptr<content::NavigationEntryRestoreContext> restore_context =
      content::NavigationEntryRestoreContext::Create();
  std::unique_ptr<content::NavigationEntry> entry =
      sessions::ContentSerializedNavigationBuilder::ToNavigationEntry(
          &navigation, &browser_context_, restore_context.get());

  ASSERT_TRUE(entry);
  auto restored_key = entry->GetStoragePartitionKeyToRestore();
  ASSERT_TRUE(restored_key.has_value());
  EXPECT_EQ(*restored_key, storage_partition_key);
}

class ContainersContentSerializedNavigationBuilderFeatureDisabledTest
    : public ContainersContentSerializedNavigationBuilderTest {
 public:
  ContainersContentSerializedNavigationBuilderFeatureDisabledTest() {
    feature_list_.InitAndDisableFeature(features::kContainers);
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(ContainersContentSerializedNavigationBuilderFeatureDisabledTest,
       ToNavigationEntryDoesNotRestorePartitionDataWhenDisabled) {
  sessions::SerializedNavigationEntry navigation;
  navigation.set_index(0);
  navigation.set_virtual_url(GURL("https://example.com"));
  navigation.set_storage_partition_key(MakeContainerStoragePartitionKey());

  std::unique_ptr<content::NavigationEntryRestoreContext> restore_context =
      content::NavigationEntryRestoreContext::Create();
  std::unique_ptr<content::NavigationEntry> entry =
      sessions::ContentSerializedNavigationBuilder::ToNavigationEntry(
          &navigation, &browser_context_, restore_context.get());

  ASSERT_TRUE(entry);
  EXPECT_FALSE(entry->GetStoragePartitionKeyToRestore().has_value());
}

}  // namespace containers
