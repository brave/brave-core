// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/storage_partition_utils.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/containers/core/common/features.h"
#include "brave/components/containers/core/mojom/containers.mojom.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace containers {

class ContainersStoragePartitionUtilsContentTest : public testing::Test {
 protected:
  base::test::ScopedFeatureList scoped_feature_list_{features::kContainers};
  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
  content::TestBrowserContext browser_context_;
};

TEST_F(ContainersStoragePartitionUtilsContentTest,
       IsContainerStoragePartition) {
  auto config = content::StoragePartitionConfig::Create(
      &browser_context_, "container-abc", "", false);
  EXPECT_TRUE(IsContainerStoragePartition(config));
  auto non_container_config = content::StoragePartitionConfig::Create(
      &browser_context_, "notcontainer-abc", "", false);
  EXPECT_FALSE(IsContainerStoragePartition(non_container_config));
}

TEST_F(ContainersStoragePartitionUtilsContentTest,
       GetContainerIdFromStoragePartition) {
  auto config = content::StoragePartitionConfig::Create(
      &browser_context_, "container-xyz", "", false);
  auto id = GetContainerIdFromStoragePartition(config);
  ASSERT_TRUE(id.has_value());
  EXPECT_EQ(*id, "xyz");
  auto non_container_config = content::StoragePartitionConfig::Create(
      &browser_context_, "notcontainer-abc", "", false);
  EXPECT_FALSE(
      GetContainerIdFromStoragePartition(non_container_config).has_value());
}

TEST_F(ContainersStoragePartitionUtilsContentTest,
       CreateContainerStoragePartition) {
  auto container = containers::mojom::Container::New();
  container->id = "testid";
  container->name = "Test Container";
  auto config = CreateContainerStoragePartition(&browser_context_, container);
  EXPECT_EQ(config.partition_domain(), "container-testid");
}

TEST_F(ContainersStoragePartitionUtilsContentTest,
       InheritContainerStoragePartition_WebContents) {
  // Create a SiteInstance with a container partition config
  auto container_config = content::StoragePartitionConfig::Create(
      &browser_context_, "container-abc", "", false);
  auto site_instance = content::SiteInstance::Create(&browser_context_);
  // We can't set the partition config directly, but we can create a WebContents
  // with this SiteInstance and test the fallback path
  // (site_instance->GetStoragePartitionConfig()). For the test, we will use the
  // default config from the SiteInstance, but the function will return nullopt
  // unless the config is a container. So, this test is mostly a smoke test for
  // the fallback path.
  auto web_contents = content::WebContentsTester::CreateTestWebContents(
      &browser_context_, site_instance);
  // The default SiteInstance config is not a container, so expect nullopt
  auto result = InheritContainerStoragePartition(web_contents.get());
  EXPECT_FALSE(result.has_value());
}

TEST_F(ContainersStoragePartitionUtilsContentTest,
       InheritContainerStoragePartition_StoragePartitionConfig) {
  auto container_config = content::StoragePartitionConfig::Create(
      &browser_context_, "container-abc", "", false);
  auto non_container_config = content::StoragePartitionConfig::Create(
      &browser_context_, "notcontainer-abc", "", false);
  auto site_instance = content::SiteInstance::Create(&browser_context_);
  // Case 1: storage_partition_config is a container partition
  auto result =
      InheritContainerStoragePartition(container_config, site_instance.get());
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->partition_domain(), "container-abc");
  // Case 2: storage_partition_config is not a container, and site_instance is
  // not a container
  auto result2 = InheritContainerStoragePartition(non_container_config,
                                                  site_instance.get());
  EXPECT_FALSE(result2.has_value());
  // Case 3: nullopt config, site_instance is not a container
  auto result3 =
      InheritContainerStoragePartition(std::nullopt, site_instance.get());
  EXPECT_FALSE(result3.has_value());
}

TEST_F(ContainersStoragePartitionUtilsContentTest,
       InheritContainerStoragePartition_SiteInstance) {
  // Create a non-default (container) StoragePartitionConfig
  auto container_config = content::StoragePartitionConfig::Create(
      &browser_context_, "container-abc", "", false);
  // Create a SiteInstance for a fixed storage partition (container)
  auto site_instance = content::SiteInstance::CreateForFixedStoragePartition(
      &browser_context_, GURL("https://example.com"), container_config);
  // Pass nullopt for the config, should pick up from SiteInstance
  auto result =
      InheritContainerStoragePartition(std::nullopt, site_instance.get());
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->partition_domain(), "container-abc");
  // If we pass a non-container config, should still pick up from SiteInstance
  auto non_container_config = content::StoragePartitionConfig::Create(
      &browser_context_, "notcontainer-abc", "", false);
  auto result2 = InheritContainerStoragePartition(non_container_config,
                                                  site_instance.get());
  ASSERT_TRUE(result2.has_value());
  EXPECT_EQ(result2->partition_domain(), "container-abc");
  // If we pass a container config, should return that
  auto result3 =
      InheritContainerStoragePartition(container_config, site_instance.get());
  ASSERT_TRUE(result3.has_value());
  EXPECT_EQ(result3->partition_domain(), "container-abc");
}

class ContainersStoragePartitionUtilsContentDisabledTest
    : public ContainersStoragePartitionUtilsContentTest {
 public:
  ContainersStoragePartitionUtilsContentDisabledTest() {
    scoped_feature_list_.InitAndDisableFeature(features::kContainers);
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(ContainersStoragePartitionUtilsContentDisabledTest,
       InheritContainerStoragePartition_StoragePartitionConfig) {
  auto container_config = content::StoragePartitionConfig::Create(
      &browser_context_, "container-abc", "", false);
  auto site_instance = content::SiteInstance::Create(&browser_context_);
  auto result =
      InheritContainerStoragePartition(container_config, site_instance.get());
  EXPECT_EQ(result, std::nullopt);
}

TEST_F(ContainersStoragePartitionUtilsContentDisabledTest,
       InheritContainerStoragePartition_SiteInstance) {
  auto container_config = content::StoragePartitionConfig::Create(
      &browser_context_, "container-abc", "", false);
  auto site_instance = content::SiteInstance::CreateForFixedStoragePartition(
      &browser_context_, GURL("https://example.com"), container_config);
  auto result =
      InheritContainerStoragePartition(std::nullopt, site_instance.get());
  EXPECT_EQ(result, std::nullopt);
}

}  // namespace containers
