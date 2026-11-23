// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_WORKSPACE_ASSOCIATED_CONTENT_BROWSERTEST_BASE_H_
#define BRAVE_BROWSER_AI_CHAT_WORKSPACE_ASSOCIATED_CONTENT_BROWSERTEST_BASE_H_

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/scoped_temp_dir.h"
#include "base/location.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/content/browser/workspace_associated_content.h"
#include "chrome/test/base/platform_browser_test.h"
#include "url/gurl.h"

namespace base {
class CommandLine;
}  // namespace base

namespace content {
class WebContents;
}  // namespace content

namespace ai_chat {

// Fixture shared by the workspace browser tests. It is a PlatformBrowserTest,
// so tests that only need what it provides also run on Android.
class WorkspaceAssociatedContentBrowserTestBase : public PlatformBrowserTest {
 public:
  // |enable_web_mcp| turns on WebMCP, which the workspace page needs to
  // register its tools.
  explicit WorkspaceAssociatedContentBrowserTestBase(
      bool enable_web_mcp = false);
  ~WorkspaceAssociatedContentBrowserTestBase() override;

  // PlatformBrowserTest:
  void SetUpCommandLine(base::CommandLine* command_line) override;
  void SetUpOnMainThread() override;
  void TearDownOnMainThread() override;

 protected:
  // Returns a fresh folder, with a file in it, for a workspace to be pointed
  // at. Tests which create more than one workspace need a folder each.
  base::FilePath CreateWorkspaceFolder();

  // Creates a workspace for |folder|, owned by the fixture so that it outlives
  // its page's IndexedDB write (see TearDownOnMainThread()). A workspace with
  // no folder only writes one once one of its tools has run.
  WorkspaceAssociatedContent* CreateContent(
      std::optional<base::FilePath> folder);

  // Evaluates to the directory handle |content|'s page has committed to
  // IndexedDB, waiting for the page to load and save it. A read transaction
  // can't run until an earlier write to the same store has committed, so once
  // this sees the handle the write is complete.
  static constexpr char kGetStoredHandleJs[] = R"JS(
      (async () => {
        const getHandle = () => new Promise((resolve, reject) => {
          const open = indexedDB.open('leo-workspace', 1);
          open.onerror = () => reject(open.error);
          open.onupgradeneeded = () => open.result.createObjectStore('handles');
          open.onsuccess = () => {
            const db = open.result;
            const request =
                db.transaction('handles').objectStore('handles').get('root');
            request.onerror = () => reject(request.error);
            request.onsuccess = () => {
              db.close();
              resolve(request.result);
            };
          };
        });
        let handle;
        while (!(handle = await getHandle())) {
          await new Promise(resolve => setTimeout(resolve, 10));
        }
        return handle;
      })()
  )JS";

  [[nodiscard]] static bool WaitForStoredHandle(
      WorkspaceAssociatedContent* content);

  // The URL of the viewer belonging to the workspace at |workspace_url|.
  static GURL ViewerURL(const GURL& workspace_url);

  // Makes `url` the sole iframe of `web_contents`' main document and returns
  // whether it ended up showing that URL's document. Blocked and permitted
  // frames alike fire `load`, so the outcome is read off the committed
  // document: frame-src leaves the frame on its initial empty document,
  // frame-ancestors commits an error document at the URL.
  static bool FrameLoads(content::WebContents* web_contents,
                         const GURL& url,
                         base::Location location = base::Location::Current());

  base::ScopedTempDir temp_dir_;

 private:
  const bool enable_web_mcp_;
  std::vector<std::unique_ptr<WorkspaceAssociatedContent>> contents_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_WORKSPACE_ASSOCIATED_CONTENT_BROWSERTEST_BASE_H_
