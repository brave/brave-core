// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/buildflags/buildflags.h"

#include <chrome/browser/ui/sync/tab_contents_synced_tab_delegate_unittest.cc>

#if BUILDFLAG(ENABLE_CONTAINERS)

#include "brave/components/containers/content/browser/storage_partition_utils.h"
#include "brave/components/containers/core/browser/temporary_container.h"
#include "brave/components/containers/core/common/features.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition_config.h"

namespace {

std::unique_ptr<content::WebContents>
CreateWebContentsWithFixedStoragePartition(
    content::BrowserContext* browser_context,
    std::string_view partition_domain,
    std::string_view partition_name,
    const GURL& url) {
  const content::StoragePartitionConfig storage_partition_config =
      content::StoragePartitionConfig::Create(
          browser_context, std::string(partition_domain),
          std::string(partition_name), browser_context->IsOffTheRecord());
  return content::WebContentsTester::CreateTestWebContents(
      browser_context, content::SiteInstance::CreateForFixedStoragePartition(
                           browser_context, url, storage_partition_config));
}

std::unique_ptr<content::WebContents> CreateContainerWebContents(
    content::BrowserContext* browser_context,
    std::string_view container_id,
    const GURL& url) {
  return CreateWebContentsWithFixedStoragePartition(
      browser_context, containers::kContainersStoragePartitionDomain,
      std::string(container_id), url);
}

TEST_F(TabContentsSyncedTabDelegateTest,
       ShouldSyncReturnsFalseForTemporaryContainerTab) {
  base::test::ScopedFeatureList feature_list(containers::features::kContainers);
  const GURL url("https://example.test/");
  std::unique_ptr<content::WebContents> web_contents =
      CreateContainerWebContents(browser_context(), "t-temporary", url);
  content::WebContentsTester::For(web_contents.get())->NavigateAndCommit(url);
  TestSyncedTabDelegate delegate(web_contents.get());
  window_getter_.AddWindow(sync_pb::SyncEnums_BrowserType_TYPE_TABBED,
                           delegate.GetWindowId());

  EXPECT_FALSE(delegate.ShouldSync(&mock_sync_sessions_client_));
}

TEST_F(TabContentsSyncedTabDelegateTest,
       ShouldSyncReturnsTrueForNonTemporaryContainerTab) {
  base::test::ScopedFeatureList feature_list(containers::features::kContainers);
  const GURL url("https://example.test/");
  std::unique_ptr<content::WebContents> web_contents =
      CreateContainerWebContents(browser_context(), "persisted-container", url);
  content::WebContentsTester::For(web_contents.get())->NavigateAndCommit(url);
  TestSyncedTabDelegate delegate(web_contents.get());
  window_getter_.AddWindow(sync_pb::SyncEnums_BrowserType_TYPE_TABBED,
                           delegate.GetWindowId());

  EXPECT_TRUE(delegate.ShouldSync(&mock_sync_sessions_client_));
}

TEST_F(TabContentsSyncedTabDelegateTest,
       ShouldSyncReturnsTrueForRegularTabWithContainersEnabled) {
  base::test::ScopedFeatureList feature_list(containers::features::kContainers);
  const GURL url("https://example.test/");
  std::unique_ptr<content::WebContents> web_contents(CreateTestWebContents());
  content::WebContentsTester::For(web_contents.get())->NavigateAndCommit(url);
  TestSyncedTabDelegate delegate(web_contents.get());
  window_getter_.AddWindow(sync_pb::SyncEnums_BrowserType_TYPE_TABBED,
                           delegate.GetWindowId());

  EXPECT_TRUE(delegate.ShouldSync(&mock_sync_sessions_client_));
}

TEST_F(TabContentsSyncedTabDelegateTest,
       ShouldSyncReturnsTrueForNonContainerStoragePartitionTab) {
  base::test::ScopedFeatureList feature_list(containers::features::kContainers);
  const GURL url("https://example.test/");
  std::unique_ptr<content::WebContents> web_contents =
      CreateWebContentsWithFixedStoragePartition(
          browser_context(), "non-container-domain", "regular-partition", url);
  content::WebContentsTester::For(web_contents.get())->NavigateAndCommit(url);
  TestSyncedTabDelegate delegate(web_contents.get());
  window_getter_.AddWindow(sync_pb::SyncEnums_BrowserType_TYPE_TABBED,
                           delegate.GetWindowId());

  EXPECT_TRUE(delegate.ShouldSync(&mock_sync_sessions_client_));
}

}  // namespace

#endif  // BUILDFLAG(ENABLE_CONTAINERS)
