// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/workspace_associated_content_browsertest_base.h"

#include <string>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/files/file_util.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/strcat.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/features.h"

namespace ai_chat {

WorkspaceAssociatedContentBrowserTestBase::
    WorkspaceAssociatedContentBrowserTestBase(bool enable_web_mcp)
    : enable_web_mcp_(enable_web_mcp) {
  std::vector<base::test::FeatureRef> enabled = {
      features::kAIChatWorkspaceTools};
  if (enable_web_mcp_) {
    enabled.push_back(blink::features::kWebMCP);
  }
  scoped_feature_list_.InitWithFeatures(enabled, {});
}

WorkspaceAssociatedContentBrowserTestBase::
    ~WorkspaceAssociatedContentBrowserTestBase() = default;

void WorkspaceAssociatedContentBrowserTestBase::SetUpCommandLine(
    base::CommandLine* command_line) {
  PlatformBrowserTest::SetUpCommandLine(command_line);
  if (enable_web_mcp_) {
    // The runtime-enabled feature gating document.modelContext is marked
    // "experimental", so the base::Feature toggle alone isn't enough to turn it
    // on in the renderer.
    command_line->AppendSwitchASCII("enable-blink-features", "WebMCP");
  }
}

void WorkspaceAssociatedContentBrowserTestBase::SetUpOnMainThread() {
  PlatformBrowserTest::SetUpOnMainThread();
  base::ScopedAllowBlockingForTesting allow_blocking;
  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
}

void WorkspaceAssociatedContentBrowserTestBase::TearDownOnMainThread() {
  // Each page saves the handle it's given to IndexedDB, which serializes it
  // asynchronously while committing. Tearing the transaction down mid-commit
  // trips the dangling pointer detector in IndexedDB's BackingStore (it binds
  // the external object Unretained, guarded only by a WeakPtr checked inside
  // the callback), so let every write land before destroying anything.
  DestroyContents();
  PlatformBrowserTest::TearDownOnMainThread();
}

void WorkspaceAssociatedContentBrowserTestBase::DestroyContents() {
  for (const auto& content : contents_) {
    EXPECT_TRUE(WaitForStoredHandle(content.get()))
        << "workspace " << content->uuid();
  }
  contents_.clear();
}

base::FilePath
WorkspaceAssociatedContentBrowserTestBase::CreateWorkspaceFolder() {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath folder;
  EXPECT_TRUE(base::CreateTemporaryDirInDir(
      temp_dir_.GetPath(), FILE_PATH_LITERAL("workspace"), &folder));
  EXPECT_TRUE(base::WriteFile(folder.AppendASCII("hello.txt"), "hello world"));
  return folder;
}

WorkspaceAssociatedContent*
WorkspaceAssociatedContentBrowserTestBase::CreateContent(
    std::optional<base::FilePath> folder) {
  return contents_
      .emplace_back(std::make_unique<WorkspaceAssociatedContent>(
          std::move(folder), chrome_test_utils::GetProfile(this),
          base::DoNothing()))
      .get();
}

// static
bool WorkspaceAssociatedContentBrowserTestBase::WaitForStoredHandle(
    WorkspaceAssociatedContent* content) {
  content::WebContents* web_contents = content->GetWebContentsForTesting();
  return content::WaitForLoadStop(web_contents) &&
         content::EvalJs(web_contents, base::StrCat({kGetStoredHandleJs,
                                                     ".then(h => h.kind)"}))
                 .ExtractString() == "directory";
}

// static
GURL WorkspaceAssociatedContentBrowserTestBase::ViewerURL(
    const GURL& workspace_url) {
  const std::string host =
      base::StrCat({kAIChatLeoWorkspaceViewUIHostPrefix, workspace_url.host()});
  GURL::Replacements replacements;
  replacements.SetHostStr(host);
  return workspace_url.ReplaceComponents(replacements);
}

// static
bool WorkspaceAssociatedContentBrowserTestBase::FrameLoads(
    content::WebContents* web_contents,
    const GURL& url,
    base::Location location) {
  SCOPED_TRACE(base::StrCat({location.ToString(), " framing ", url.spec()}));
  EXPECT_TRUE(content::ExecJs(web_contents, content::JsReplace(R"JS(
      new Promise(resolve => {
        document.querySelectorAll('iframe').forEach(frame => frame.remove());
        const frame = document.createElement('iframe');
        frame.addEventListener('load', () => resolve());
        frame.src = $1;
        document.body.appendChild(frame);
      })
  )JS",
                                                               url)));
  content::RenderFrameHost* child =
      content::ChildFrameAt(web_contents->GetPrimaryMainFrame(), 0);
  return child && child->GetLastCommittedURL() == url &&
         !child->IsErrorDocument();
}

}  // namespace ai_chat
