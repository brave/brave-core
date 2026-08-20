// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/workspace_associated_content.h"

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

// Covers the browser-side half of the workspace pipeline: the hidden
// chrome-untrusted://leo-workspace page is created and loaded, the workspace
// origin is granted File System Access, and the delegate reports itself as a
// tool host. The page's own tool registration (WebMCP) is covered separately by
// the workspace tools browser test, which needs the workspace bundle.
class WorkspaceAssociatedContentBrowserTest : public InProcessBrowserTest {
 public:
  WorkspaceAssociatedContentBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kAIChatWorkspaceTools);
  }
  ~WorkspaceAssociatedContentBrowserTest() override = default;

 protected:
  base::FilePath CreateWorkspaceFolder() {
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());
    const base::FilePath root = temp_dir_.GetPath();
    EXPECT_TRUE(base::WriteFile(root.AppendASCII("hello.txt"), "hello world"));
    return root;
  }

  std::unique_ptr<WorkspaceAssociatedContent> CreateContent(
      const base::FilePath& folder) {
    return std::make_unique<WorkspaceAssociatedContent>(
        folder, browser()->GetProfile(), base::DoNothing());
  }

  ContentSetting GetSetting(const GURL& url, ContentSettingsType type) {
    return HostContentSettingsMapFactory::GetForProfile(browser()->GetProfile())
        ->GetContentSetting(url, url, type);
  }

  base::ScopedTempDir temp_dir_;

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(WorkspaceAssociatedContentBrowserTest,
                       LoadsHiddenWorkspacePageForPickedFolder) {
  const base::FilePath folder = CreateWorkspaceFolder();
  auto content = CreateContent(folder);

  EXPECT_EQ(folder, content->folder_path());

  // Each workspace gets its own chrome-untrusted://leo-workspace/<uuid> URL so
  // conversations don't share a document.
  const GURL url = content->url();
  EXPECT_TRUE(url.SchemeIs(content::kChromeUIUntrustedScheme));
  EXPECT_EQ(kAIChatLeoWorkspaceUIHost, url.host());
  EXPECT_FALSE(content->uuid().empty());
  EXPECT_EQ("/" + content->uuid(), url.path());

  // The page is a headless tool host: it must never be visible to the user.
  content::WebContents* web_contents = content->GetWebContentsForTesting();
  ASSERT_TRUE(web_contents);
  EXPECT_EQ(content::Visibility::HIDDEN, web_contents->GetVisibility());

  ASSERT_TRUE(content::WaitForLoadStop(web_contents));
  EXPECT_EQ(url, web_contents->GetLastCommittedURL());

  // Once loaded, the delegate is a live tool host, so the next generation loop
  // harvests whatever the page registered.
  EXPECT_TRUE(base::test::RunUntil([&] { return content->tools_attached(); }));
}

IN_PROC_BROWSER_TEST_F(WorkspaceAssociatedContentBrowserTest,
                       GrantsFileSystemAccessToWorkspaceOriginOnLoad) {
  const GURL workspace_url(kAIChatLeoWorkspaceUIURL);
  ASSERT_EQ(
      CONTENT_SETTING_ASK,
      GetSetting(workspace_url, ContentSettingsType::FILE_SYSTEM_READ_GUARD));
  ASSERT_EQ(
      CONTENT_SETTING_ASK,
      GetSetting(workspace_url, ContentSettingsType::FILE_SYSTEM_WRITE_GUARD));

  auto content = CreateContent(CreateWorkspaceFolder());
  ASSERT_TRUE(content::WaitForLoadStop(content->GetWebContentsForTesting()));
  ASSERT_TRUE(base::test::RunUntil([&] { return content->tools_attached(); }));

  // The handle is delivered without a permission prompt, which requires the
  // workspace origin to hold read/write File System Access grants.
  EXPECT_EQ(
      CONTENT_SETTING_ALLOW,
      GetSetting(workspace_url, ContentSettingsType::FILE_SYSTEM_READ_GUARD));
  EXPECT_EQ(
      CONTENT_SETTING_ALLOW,
      GetSetting(workspace_url, ContentSettingsType::FILE_SYSTEM_WRITE_GUARD));
}

IN_PROC_BROWSER_TEST_F(WorkspaceAssociatedContentBrowserTest,
                       ContributesNoPageTextToTheConversation) {
  auto content = CreateContent(CreateWorkspaceFolder());
  ASSERT_TRUE(content::WaitForLoadStop(content->GetWebContentsForTesting()));

  // The value of this content is its tools, not its text: sending the
  // workspace page's markup to the model would be noise.
  base::test::TestFuture<PageContent> page_content;
  content->GetContent(page_content.GetCallback());
  EXPECT_EQ(PageContent(), page_content.Get());
}

IN_PROC_BROWSER_TEST_F(WorkspaceAssociatedContentBrowserTest,
                       ReportsNoToolsBeforeThePageIsReady) {
  auto content = CreateContent(CreateWorkspaceFolder());

  // Before the page loads, GetContentTools must reply synchronously and empty:
  // a late reply for the initial (about:blank) document would clobber the
  // attach done on load.
  base::test::TestFuture<std::vector<std::unique_ptr<Tool>>> tools;
  content->GetContentTools(tools.GetCallback());
  ASSERT_TRUE(tools.IsReady());
  EXPECT_TRUE(tools.Take().empty());
  EXPECT_FALSE(content->tools_attached());
}

IN_PROC_BROWSER_TEST_F(WorkspaceAssociatedContentBrowserTest,
                       DestroyingContentBeforeLoadDoesNotCrash) {
  auto content = CreateContent(CreateWorkspaceFolder());
  // The navigation started in the constructor is still in flight.
  content.reset();
}

}  // namespace ai_chat
