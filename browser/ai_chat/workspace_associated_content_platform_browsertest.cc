// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/json/json_writer.h"
#include "base/strings/strcat.h"
#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/browser/ai_chat/workspace_associated_content_browsertest_base.h"
#include "brave/components/ai_chat/content/browser/workspace_associated_content.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

// Workspace tests that also run on Android. They use the kind of workspace
// Android has, one backed by its origin private file system (OPFS) rather than
// a folder the user picked, and fill it through the workspace's own tools.
class WorkspaceAssociatedContentPlatformBrowserTest
    : public WorkspaceAssociatedContentBrowserTestBase {
 public:
  WorkspaceAssociatedContentPlatformBrowserTest()
      : WorkspaceAssociatedContentBrowserTestBase(/*enable_web_mcp=*/true) {}

 protected:
  // Creates an OPFS-backed workspace and waits for its page to attach its
  // tools.
  WorkspaceAssociatedContent* CreateOpfsWorkspace() {
    WorkspaceAssociatedContent* content = CreateContent(std::nullopt);
    EXPECT_TRUE(
        base::test::RunUntil([&] { return content->tools_attached(); }));
    return content;
  }

  // Runs |content|'s tool |name| on |input| and returns its text output.
  static std::string RunTool(WorkspaceAssociatedContent* content,
                             std::string_view name,
                             const base::DictValue& input) {
    base::test::TestFuture<std::vector<std::unique_ptr<Tool>>> tools;
    content->GetContentTools(tools.GetCallback());
    for (const auto& tool : tools.Get()) {
      if (tool->Name() != name) {
        continue;
      }
      base::test::TestFuture<Tool::ToolResult, Tool::ToolArtifacts> result;
      tool->UseTool(*base::WriteJson(input), result.GetCallback());
      const Tool::ToolResult& output = result.Get<0>();
      if (output.size() != 1u || !output[0]->is_text_content_block()) {
        ADD_FAILURE() << name << " returned no text";
        return std::string();
      }
      return output[0]->get_text_content_block()->text;
    }
    ADD_FAILURE() << "no tool named " << name;
    return std::string();
  }

  // Writes |text| to the workspace file |path|. This is also what makes an
  // OPFS-backed workspace adopt its OPFS as its folder, which it otherwise only
  // does once one of its tools needs a folder.
  static void CreateFile(WorkspaceAssociatedContent* content,
                         std::string_view path,
                         std::string_view text) {
    EXPECT_THAT(RunTool(content, "str_replace_based_edit_tool",
                        base::DictValue()
                            .Set("command", "create")
                            .Set("path", path)
                            .Set("file_text", text)),
                ::testing::StartsWith(base::StrCat({"Created ", path})));
  }
};

// The viewer serves a file from the path of its URL, so a file name containing
// characters that are special in a URL has to be escaped on the way in, and
// unescaped by the service worker, to reach the right file. Unescaped, the `#`
// here would cut the name short, and the service worker would look for
// "a%20b".
IN_PROC_BROWSER_TEST_F(WorkspaceAssociatedContentPlatformBrowserTest,
                       ViewerDisplaysFileWhoseNameNeedsEscaping) {
  auto* content = CreateOpfsWorkspace();
  CreateFile(content, "a b#1.txt", "escaped");
  content::WebContents* web_contents = content->GetWebContentsForTesting();

  const GURL viewer_url = ViewerURL(content->page_url());
  ASSERT_TRUE(
      FrameLoads(web_contents, viewer_url.Resolve("/#file=a%20b%231.txt")));
  content::RenderFrameHost* viewer =
      content::ChildFrameAt(web_contents->GetPrimaryMainFrame(), 0);
  ASSERT_TRUE(viewer);

  // The viewer frames the file once its service worker is ready.
  content::RenderFrameHost* file = nullptr;
  ASSERT_TRUE(base::test::RunUntil([&] {
    file = content::ChildFrameAt(viewer, 0);
    return file && file->GetLastCommittedURL().path() == "/a%20b%231.txt";
  }));
  EXPECT_EQ(viewer_url.Resolve("/a%20b%231.txt"), file->GetLastCommittedURL());
  EXPECT_EQ("escaped", content::EvalJs(file, R"JS(
      new Promise(resolve => {
        const text = () => resolve(document.body.innerText);
        if (document.readyState === 'complete') {
          text();
        } else {
          addEventListener('load', text);
        }
      })
  )JS"));
}

}  // namespace ai_chat
