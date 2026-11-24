// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/strings/strcat.h"
#include "base/strings/string_view_util.h"
#include "base/test/run_until.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/browser/ai_chat/workspace_associated_content_browsertest_base.h"
#include "brave/browser/ui/webui/ai_chat/leo_workspace_page_handler.h"
#include "brave/browser/ui/webui/ai_chat/leo_workspace_ui.h"
#include "brave/components/ai_chat/content/browser/workspace_associated_content.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/leo_workspace.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_controller.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "printing/buildflags/buildflags.h"
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

  // Frames the viewer of |web_contents|' workspace, as a frame named |name|,
  // displaying |file|, and waits for the viewer to report that it has.
  static void DisplayFile(content::WebContents* web_contents,
                          const GURL& workspace_url,
                          std::string_view name,
                          std::string_view file,
                          base::Location location = base::Location::Current()) {
    SCOPED_TRACE(location.ToString());
    EXPECT_EQ(file, content::EvalJs(
                        web_contents,
                        content::JsReplace(
                            R"JS(
        new Promise(resolve => {
          addEventListener('message', event => {
            if (event.data?.type === 'FILE_DISPLAYED') {
              resolve(event.data.file);
            }
          });
          const frame = document.createElement('iframe');
          frame.name = $1;
          frame.src = $2;
          document.body.appendChild(frame);
        })
    )JS",
                            name,
                            ViewerURL(workspace_url)
                                .Resolve(base::StrCat({"/#file=", file})))));
  }

  // The page size of the first page of |pdf|, in points, as "0 0 <w> <h>".
  static std::string PdfMediaBox(std::string_view pdf) {
    static constexpr std::string_view kMediaBox = "/MediaBox [";
    const size_t media_box = pdf.find(kMediaBox);
    if (media_box == std::string_view::npos) {
      return std::string();
    }
    const size_t start = media_box + kMediaBox.size();
    return std::string(pdf.substr(start, pdf.find(']', start) - start));
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

// The browser prints a workspace file by finding the frame that displays it:
// the file frame inside the viewer the workspace named, and nothing else.
IN_PROC_BROWSER_TEST_F(WorkspaceAssociatedContentPlatformBrowserTest,
                       FindsOnlyTheNamedViewersFileFrame) {
  auto* content = CreateOpfsWorkspace();
  CreateFile(content, "hello.txt", "hello world");
  content::WebContents* web_contents = content->GetWebContentsForTesting();
  content::RenderFrameHost* workspace = web_contents->GetPrimaryMainFrame();

  DisplayFile(web_contents, content->page_url(), "viewer", "hello.txt");
  content::RenderFrameHost* viewer = content::ChildFrameAt(workspace, 0);
  ASSERT_TRUE(viewer);
  content::RenderFrameHost* file = content::ChildFrameAt(viewer, 0);
  ASSERT_TRUE(file);
  EXPECT_EQ(ViewerURL(content->page_url()).Resolve("/hello.txt"),
            file->GetLastCommittedURL());
  EXPECT_EQ(file, FindLeoWorkspaceFileFrame(workspace, "viewer"));

  EXPECT_FALSE(FindLeoWorkspaceFileFrame(workspace, "other"));
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(workspace, ""));
  // Only ever looked for inside the workspace's own document.
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(viewer, "viewer"));
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(file, "viewer"));
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(nullptr, "viewer"));

  // A viewer that displays no file has nothing to print.
  ASSERT_TRUE(content::ExecJs(
      web_contents, content::JsReplace(R"JS(
      new Promise(resolve => {
        const frame = document.createElement('iframe');
        frame.name = 'empty';
        frame.onload = () => resolve();
        frame.src = $1;
        document.body.appendChild(frame);
      })
  )JS",
                                       ViewerURL(content->page_url()))));
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(workspace, "empty"));

  // Nor is a frame the workspace named which isn't its viewer.
  ASSERT_TRUE(content::ExecJs(web_contents, R"JS(
      new Promise(resolve => {
        const frame = document.createElement('iframe');
        frame.name = 'impostor';
        frame.onload = () => resolve();
        frame.src = 'about:blank';
        document.body.appendChild(frame);
      })
  )JS"));
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(workspace, "impostor"));
}

#if BUILDFLAG(ENABLE_PRINTING)
// The workspace page asks the browser to print the file its named viewer
// displays, and gets back that file, as laid out by Blink, as a PDF. The file
// sets its own page size, so the PDF's page size shows which document was
// printed.
IN_PROC_BROWSER_TEST_F(WorkspaceAssociatedContentPlatformBrowserTest,
                       PrintsTheDisplayedFileToPdf) {
  auto* content = CreateOpfsWorkspace();
  CreateFile(content, "page.html",
             "<!doctype html><style>@page { size: 2in 3in; margin: 0 }</style>"
             "<p>Hello, PDF</p>");
  content::WebContents* web_contents = content->GetWebContentsForTesting();
  DisplayFile(web_contents, content->page_url(), "viewer", "page.html");

  // Bound the way the page binds it.
  auto* ui = web_contents->GetWebUI()->GetController()->GetAs<LeoWorkspaceUI>();
  ASSERT_TRUE(ui);
  mojo::Remote<mojom::LeoWorkspaceHandler> handler;
  ui->BindInterface(handler.BindNewPipeAndPassReceiver());

  {
    base::test::TestFuture<base::expected<mojo_base::BigBuffer, std::string>>
        printed;
    handler->PrintFileToPdf("viewer", /*landscape=*/false,
                            printed.GetCallback());
    const auto& pdf = printed.Get();
    ASSERT_TRUE(pdf.has_value()) << pdf.error();
    const std::string_view bytes = base::as_string_view(pdf->byte_span());
    EXPECT_TRUE(bytes.starts_with("%PDF-"));
    // 2x3 inches is 144x216 points.
    EXPECT_EQ("0 0 144 216", PdfMediaBox(bytes));
  }

  // A viewer the page hasn't framed has nothing to print.
  {
    base::test::TestFuture<base::expected<mojo_base::BigBuffer, std::string>>
        printed;
    handler->PrintFileToPdf("other", /*landscape=*/false,
                            printed.GetCallback());
    EXPECT_EQ(base::unexpected("The file to print is not being displayed"),
              printed.Get());
  }

  // A file the viewer couldn't load is displayed as the service worker's error
  // page, which must not be printed in its place.
  DisplayFile(web_contents, content->page_url(), "missing", "missing.html");
  {
    base::test::TestFuture<base::expected<mojo_base::BigBuffer, std::string>>
        printed;
    handler->PrintFileToPdf("missing", /*landscape=*/false,
                            printed.GetCallback());
    EXPECT_EQ(
        base::unexpected("The file to print could not be loaded (HTTP 404)"),
        printed.Get());
  }
}
#endif  // BUILDFLAG(ENABLE_PRINTING)

}  // namespace ai_chat
