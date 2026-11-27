// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/leo_workspace_page_handler.h"

#include <string>
#include <string_view>

#include "base/memory/scoped_refptr.h"
#include "base/strings/strcat.h"
#include "base/test/test_future.h"
#include "base/types/expected.h"
#include "brave/components/ai_chat/core/common/constants.h"
#include "brave/components/ai_chat/core/common/mojom/leo_workspace.mojom.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/test_web_ui.h"
#include "mojo/public/cpp/base/big_buffer.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_version.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ai_chat {

namespace {

constexpr char kWorkspaceId[] = "abc";

// chrome-untrusted://<host>|path|.
GURL UntrustedURL(std::string_view host, std::string_view path) {
  return GURL(base::StrCat({content::kChromeUIUntrustedScheme,
                            url::kStandardSchemeSeparator, host, path}));
}

// The workspace |id|'s page, chrome-untrusted://<id>.leo-workspace|path|.
GURL WorkspaceURL(std::string_view id, std::string_view path = "/") {
  return UntrustedURL(base::StrCat({id, kAIChatLeoWorkspaceUIHostSuffix}),
                      path);
}

// The workspace |id|'s viewer,
// chrome-untrusted://view.<id>.leo-workspace|path|.
GURL ViewerURL(std::string_view id, std::string_view path = "/") {
  return UntrustedURL(base::StrCat({kAIChatLeoWorkspaceViewUIHostPrefix, id,
                                    kAIChatLeoWorkspaceUIHostSuffix}),
                      path);
}

}  // namespace

class LeoWorkspacePageHandlerTest : public ChromeRenderViewHostTestHarness {
 protected:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    NavigateAndCommit(WorkspaceURL(kWorkspaceId));
  }

  content::RenderFrameHost* workspace() { return main_rfh(); }

  // Appends a child named |name| to |parent| and navigates it to |url|.
  static content::RenderFrameHost* AppendFrame(content::RenderFrameHost* parent,
                                               std::string_view name,
                                               const GURL& url) {
    content::RenderFrameHost* child =
        content::RenderFrameHostTester::For(parent)->AppendChild(
            std::string(name));
    return content::NavigationSimulator::NavigateAndCommitFromDocument(url,
                                                                       child);
  }

  // Appends a child to |parent| and navigates it to |url|, which responds with
  // |status|, e.g. "404 Not Found".
  static content::RenderFrameHost* AppendFrameWithStatus(
      content::RenderFrameHost* parent,
      const GURL& url,
      std::string_view status) {
    content::RenderFrameHost* child =
        content::RenderFrameHostTester::For(parent)->AppendChild("");
    auto navigation =
        content::NavigationSimulator::CreateRendererInitiated(url, child);
    navigation->SetResponseHeaders(
        net::HttpResponseHeaders::Builder(net::HttpVersion(1, 1), status)
            .Build());
    navigation->Commit();
    return navigation->GetFinalRenderFrameHost();
  }

  // Frames the workspace's viewer, named |name|, displaying |file|. Returns the
  // file's frame.
  content::RenderFrameHost* DisplayFile(std::string_view name,
                                        std::string_view file) {
    content::RenderFrameHost* viewer =
        AppendFrame(workspace(), name,
                    ViewerURL(kWorkspaceId, base::StrCat({"/#file=", file})));
    return AppendFrame(viewer, "",
                       ViewerURL(kWorkspaceId, base::StrCat({"/", file})));
  }

  // Asks the handler for the workspace to print the file in the viewer
  // |viewer_frame_name|.
  base::expected<mojo_base::BigBuffer, std::string> PrintFileToPdf(
      std::string_view viewer_frame_name) {
    content::TestWebUI web_ui;
    web_ui.set_web_contents(web_contents());
    web_ui.set_render_frame_host(workspace());
    mojo::Remote<mojom::LeoWorkspaceHandler> remote;
    LeoWorkspacePageHandler handler(&web_ui,
                                    remote.BindNewPipeAndPassReceiver());
    base::test::TestFuture<base::expected<mojo_base::BigBuffer, std::string>>
        printed;
    handler.PrintFileToPdf(std::string(viewer_frame_name), /*landscape=*/false,
                           printed.GetCallback());
    return printed.Take();
  }
};

TEST_F(LeoWorkspacePageHandlerTest, FindsTheFileFrameOfTheNamedViewer) {
  DisplayFile("first", "a.txt");
  content::RenderFrameHost* file = DisplayFile("second", "b.txt");
  EXPECT_EQ(ViewerURL(kWorkspaceId, "/b.txt"), file->GetLastCommittedURL());
  EXPECT_EQ(file, FindLeoWorkspaceFileFrame(workspace(), "second"));
}

TEST_F(LeoWorkspacePageHandlerTest, FindsNothingForAnotherName) {
  DisplayFile("viewer", "a.txt");
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(workspace(), "other"));
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(workspace(), ""));
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(nullptr, "viewer"));
}

// Only the workspace's own document can print its files.
TEST_F(LeoWorkspacePageHandlerTest, FindsNothingFromOtherThanTheWorkspace) {
  content::RenderFrameHost* file = DisplayFile("viewer", "a.txt");
  content::RenderFrameHost* viewer = file->GetParent();
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(viewer, "viewer"));
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(file, "viewer"));

  // A page that isn't a workspace, framing a workspace's viewer.
  content::RenderFrameHost* page =
      AppendFrame(workspace(), "page", UntrustedURL("example", "/"));
  content::RenderFrameHost* page_viewer =
      AppendFrame(page, "viewer", ViewerURL(kWorkspaceId, "/#file=a.txt"));
  AppendFrame(page_viewer, "", ViewerURL(kWorkspaceId, "/a.txt"));
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(page, "viewer"));
}

// A frame the workspace named which isn't its own viewer is never printed from.
TEST_F(LeoWorkspacePageHandlerTest, FindsNothingInAFrameThatIsNotTheViewer) {
  // Another workspace's viewer.
  content::RenderFrameHost* other =
      AppendFrame(workspace(), "other", ViewerURL("def", "/#file=a.txt"));
  AppendFrame(other, "", ViewerURL("def", "/a.txt"));
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(workspace(), "other"));

  // A frame on the workspace's own origin.
  content::RenderFrameHost* self =
      AppendFrame(workspace(), "self", WorkspaceURL(kWorkspaceId));
  AppendFrame(self, "", ViewerURL(kWorkspaceId, "/a.txt"));
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(workspace(), "self"));

  // The workspace's viewer, but nested inside another frame rather than framed
  // by the workspace itself.
  content::RenderFrameHost* outer =
      AppendFrame(workspace(), "outer", ViewerURL(kWorkspaceId));
  content::RenderFrameHost* nested =
      AppendFrame(outer, "nested", ViewerURL(kWorkspaceId, "/#file=a.txt"));
  AppendFrame(nested, "", ViewerURL(kWorkspaceId, "/a.txt"));
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(workspace(), "nested"));
}

// A viewer only displays a file in a child frame loaded from a file's path.
TEST_F(LeoWorkspacePageHandlerTest, FindsNothingInAViewerShowingNoFile) {
  // No child frame at all.
  AppendFrame(workspace(), "empty", ViewerURL(kWorkspaceId));
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(workspace(), "empty"));

  // A child frame that hasn't loaded anything.
  content::RenderFrameHost* blank =
      AppendFrame(workspace(), "blank", ViewerURL(kWorkspaceId));
  content::RenderFrameHostTester::For(blank)->AppendChild("");
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(workspace(), "blank"));

  // A child frame showing the viewer itself, rather than a file.
  content::RenderFrameHost* self =
      AppendFrame(workspace(), "self", ViewerURL(kWorkspaceId));
  AppendFrame(self, "", ViewerURL(kWorkspaceId));
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(workspace(), "self"));

  // A child frame on another origin.
  content::RenderFrameHost* other =
      AppendFrame(workspace(), "other", ViewerURL(kWorkspaceId));
  AppendFrame(other, "", UntrustedURL("example", "/a.txt"));
  EXPECT_FALSE(FindLeoWorkspaceFileFrame(workspace(), "other"));
}

TEST_F(LeoWorkspacePageHandlerTest, PrintsNothingForAViewerNotDisplayed) {
  DisplayFile("viewer", "a.txt");
  EXPECT_EQ(base::unexpected("The file to print is not being displayed"),
            PrintFileToPdf("other"));
}

// The viewer's service worker answers a file it can't read with an error page,
// which isn't printed as if it were the file.
TEST_F(LeoWorkspacePageHandlerTest, PrintsNothingForAFileThatFailedToLoad) {
  content::RenderFrameHost* viewer = AppendFrame(
      workspace(), "viewer", ViewerURL(kWorkspaceId, "/#file=missing.html"));
  content::RenderFrameHost* file = AppendFrameWithStatus(
      viewer, ViewerURL(kWorkspaceId, "/missing.html"), "404 Not Found");
  ASSERT_EQ(file, FindLeoWorkspaceFileFrame(workspace(), "viewer"));
  EXPECT_EQ(
      base::unexpected("The file to print could not be loaded (HTTP 404)"),
      PrintFileToPdf("viewer"));
}

}  // namespace ai_chat
