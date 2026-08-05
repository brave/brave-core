/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/containers/buildflags/buildflags.h"

#include <chrome/browser/enterprise/platform_auth/platform_auth_navigation_throttle_unittest.cc>

#if BUILDFLAG(ENABLE_CONTAINERS)

#include <string>
#include <string_view>

#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/common/features.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition_config.h"

namespace enterprise_auth {

namespace {

std::unique_ptr<content::WebContents> CreateContainerWebContents(
    content::BrowserContext* browser_context,
    std::string_view container_id,
    const GURL& url) {
  const content::StoragePartitionConfig storage_partition_config =
      content::StoragePartitionConfig::Create(
          browser_context, containers::kContainersStoragePartitionDomain,
          std::string(container_id), browser_context->IsOffTheRecord());
  return content::WebContentsTester::CreateTestWebContents(
      browser_context, content::SiteInstance::CreateForFixedStoragePartition(
                           browser_context, url, storage_partition_config));
}

}  // namespace

TEST_F(PlatformAuthNavigationThrottleTest,
       MaybeCreateAndAdd_SkipsContainerTabs) {
  base::test::ScopedFeatureList feature_list(containers::features::kContainers);

  EXPECT_CALL(*mock_provider(), FetchOrigins(_))
      .WillOnce([](PlatformAuthProvider::FetchOriginsCallback callback) {
        std::move(callback).Run(nullptr);
      });
  ScopedSetProviderForTesting set_provider(TakeProvider());
  EnableManager(manager(), true);

  const GURL url("https://www.example.test/");
  std::unique_ptr<content::WebContents> container_web_contents =
      CreateContainerWebContents(&profile_, "test-container", url);

  content::MockNavigationHandle test_handle(
      url, container_web_contents->GetPrimaryMainFrame());
  auto registry = CreateRegistryWithThrottle(&test_handle);
  EXPECT_EQ(registry->throttles().size(), 0u);

  // Reset singleton so later tests see a disabled manager.
  EnableManager(manager(), false);
}

TEST_F(PlatformAuthNavigationThrottleTest,
       MaybeCreateAndAdd_CreatesThrottleForNonContainerTabs) {
  base::test::ScopedFeatureList feature_list(containers::features::kContainers);

  EXPECT_CALL(*mock_provider(), FetchOrigins(_))
      .WillOnce([](PlatformAuthProvider::FetchOriginsCallback callback) {
        std::move(callback).Run(nullptr);
      });
  ScopedSetProviderForTesting set_provider(TakeProvider());
  EnableManager(manager(), true);

  content::MockNavigationHandle test_handle(GURL("https://www.example.test/"),
                                            main_frame());
  auto registry = CreateRegistryWithThrottle(&test_handle);
  EXPECT_EQ(registry->throttles().size(), 1u);

  EnableManager(manager(), false);
}

}  // namespace enterprise_auth

#endif  // BUILDFLAG(ENABLE_CONTAINERS)
