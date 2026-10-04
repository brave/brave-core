// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/content/browser/storage_partition_utils.h"

#include "base/test/scoped_feature_list.h"
#include "base/types/optional_ref.h"
#include "brave/components/containers/content/browser/containers_web_contents_user_data.h"
#include "brave/components/containers/core/common/features.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition_config.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "content/test/storage_partition_test_helpers.h"
#include "content/test/test_web_contents.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

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
      if (test_case.expected_result) {
        EXPECT_EQ(GetContainerIdFromStoragePartitionConfig(config),
                  test_case.partition_name);
      } else {
        EXPECT_TRUE(GetContainerIdFromStoragePartitionConfig(config).empty());
      }
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

class ContainersStoragePartitionUtilsWebContentsTest : public testing::Test {
 public:
  ContainersStoragePartitionUtilsWebContentsTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kContainers);
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
  content::BrowserTaskEnvironment task_environment_;
  content::TestBrowserContext browser_context_;
};

TEST_F(ContainersStoragePartitionUtilsWebContentsTest,
       GetContainerIdForWebContents_FromContainersSiteInstance) {
  auto config = content::CreateStoragePartitionConfigForTesting(
      /*in_memory=*/false,
      /*partition_domain=*/kContainersStoragePartitionDomain,
      /*partition_name=*/"site-instance-container");

  scoped_refptr<content::SiteInstance> site =
      content::SiteInstance::CreateForFixedStoragePartition(
          &browser_context_, GURL("https://example.com/"), config);
  std::unique_ptr<content::TestWebContents> contents =
      content::TestWebContents::Create(&browser_context_, site);

  EXPECT_EQ(GetContainerIdForWebContents(contents.get()),
            "site-instance-container");
}

TEST_F(ContainersStoragePartitionUtilsWebContentsTest,
       GetContainerIdForWebContents_FromUserDataWhenStubSiteInstance) {
  scoped_refptr<content::SiteInstance> site =
      content::SiteInstance::Create(&browser_context_);
  std::unique_ptr<content::TestWebContents> contents =
      content::TestWebContents::Create(&browser_context_, site);

  ContainersWebContentsUserData::CreateForWebContents(contents.get(),
                                                      "stub-user-data-id");

  EXPECT_EQ(GetContainerIdForWebContents(contents.get()), "stub-user-data-id");
}

TEST_F(ContainersStoragePartitionUtilsWebContentsTest,
       GetContainerIdForWebContents_SiteInstancePrecedesUserData) {
  auto config = content::CreateStoragePartitionConfigForTesting(
      /*in_memory=*/false,
      /*partition_domain=*/kContainersStoragePartitionDomain,
      /*partition_name=*/"site-wins");

  scoped_refptr<content::SiteInstance> site =
      content::SiteInstance::CreateForFixedStoragePartition(
          &browser_context_, GURL("https://example.com/"), config);
  std::unique_ptr<content::TestWebContents> contents =
      content::TestWebContents::Create(&browser_context_, site);

  ContainersWebContentsUserData::CreateForWebContents(contents.get(),
                                                      "user-data-stale");

  EXPECT_EQ(GetContainerIdForWebContents(contents.get()), "site-wins");
}

TEST_F(ContainersStoragePartitionUtilsWebContentsTest,
       CopyContainerStateForDiscardedContents_FromSiteInstance) {
  auto config = content::CreateStoragePartitionConfigForTesting(
      /*in_memory=*/false,
      /*partition_domain=*/kContainersStoragePartitionDomain,
      /*partition_name=*/"discarded-copy");

  scoped_refptr<content::SiteInstance> container_site =
      content::SiteInstance::CreateForFixedStoragePartition(
          &browser_context_, GURL("https://example.com/"), config);
  std::unique_ptr<content::TestWebContents> old_contents =
      content::TestWebContents::Create(&browser_context_, container_site);

  scoped_refptr<content::SiteInstance> default_site =
      content::SiteInstance::Create(&browser_context_);
  std::unique_ptr<content::TestWebContents> new_contents =
      content::TestWebContents::Create(&browser_context_, default_site);

  ContainersWebContentsUserData::CopyContainerStateForDiscardedContents(
      old_contents.get(), new_contents.get());

  EXPECT_EQ(GetContainerIdForWebContents(new_contents.get()), "discarded-copy");
}

TEST_F(ContainersStoragePartitionUtilsWebContentsTest,
       CopyContainerStateForDiscardedContents_FromOldUserData) {
  scoped_refptr<content::SiteInstance> default_site =
      content::SiteInstance::Create(&browser_context_);
  std::unique_ptr<content::TestWebContents> old_contents =
      content::TestWebContents::Create(&browser_context_, default_site);
  ContainersWebContentsUserData::CreateForWebContents(old_contents.get(),
                                                      "chained-user-data");

  std::unique_ptr<content::TestWebContents> new_contents =
      content::TestWebContents::Create(&browser_context_, default_site);

  ContainersWebContentsUserData::CopyContainerStateForDiscardedContents(
      old_contents.get(), new_contents.get());

  EXPECT_EQ(GetContainerIdForWebContents(new_contents.get()),
            "chained-user-data");
}

}  // namespace containers
