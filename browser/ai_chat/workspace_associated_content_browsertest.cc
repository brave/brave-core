// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/content/browser/workspace_associated_content.h"

#include <memory>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/strings/strcat.h"
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
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/features.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace ai_chat {

// Covers the browser-side half of the workspace pipeline: the hidden
// chrome-untrusted://<uuid>.leo-workspace page is created and loaded, the
// workspace origin is granted File System Access, the workspace frames its own
// viewer at chrome-untrusted://view.<uuid>.leo-workspace and nothing else, and
// the delegate reports itself as a tool host. The page's own tool registration
// (WebMCP) is covered separately by the workspace tools browser test, which
// needs the workspace bundle.
class WorkspaceAssociatedContentBrowserTest : public InProcessBrowserTest {
 public:
  WorkspaceAssociatedContentBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(features::kAIChatWorkspaceTools);
  }
  ~WorkspaceAssociatedContentBrowserTest() override = default;

 protected:
  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

  // Returns a fresh folder, with a file in it, for a workspace to be pointed
  // at. Tests which create more than one workspace need a folder each.
  base::FilePath CreateWorkspaceFolder() {
    base::ScopedAllowBlockingForTesting allow_blocking;
    base::FilePath folder;
    EXPECT_TRUE(base::CreateTemporaryDirInDir(
        temp_dir_.GetPath(), FILE_PATH_LITERAL("workspace"), &folder));
    EXPECT_TRUE(
        base::WriteFile(folder.AppendASCII("hello.txt"), "hello world"));
    return folder;
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

  static GURL ViewerURL(const GURL& workspace_url) {
    const std::string host = base::StrCat(
        {kAIChatLeoWorkspaceViewUIHostPrefix, workspace_url.host()});
    GURL::Replacements replacements;
    replacements.SetHostStr(host);
    return workspace_url.ReplaceComponents(replacements);
  }

  // Makes `url` the sole iframe of `web_contents`' main document and returns
  // whether it ended up showing that URL's document. Blocked and permitted
  // frames alike fire `load`, so the outcome is read off the committed
  // document: frame-src leaves the frame on its initial empty document,
  // frame-ancestors commits an error document at the URL.
  static bool FrameLoads(content::WebContents* web_contents,
                         const GURL& url,
                         base::Location location = base::Location::Current()) {
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

  // Navigates the active tab to `url`, returning whether a document at that URL
  // actually loaded. `ui_test_utils::NavigateToURL()` can't be used for this,
  // as it reports success for committed error pages too.
  [[nodiscard]] bool NavigateActiveTabAndGetSuccess(
      const GURL& url,
      base::Location location = base::Location::Current()) {
    SCOPED_TRACE(
        base::StrCat({location.ToString(), " navigating to ", url.spec()}));
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    content::TestNavigationObserver observer(web_contents);
    EXPECT_TRUE(ui_test_utils::NavigateToURL(browser(), url));
    observer.Wait();
    return observer.last_navigation_succeeded();
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

  // Each workspace is served from its own
  // chrome-untrusted://<uuid>.leo-workspace subdomain, so no two conversations
  // share an origin (and therefore neither storage nor File System Access
  // grants).
  const GURL url = content->url();
  EXPECT_TRUE(url.SchemeIs(content::kChromeUIUntrustedScheme));
  EXPECT_FALSE(content->uuid().empty());
  EXPECT_EQ(content->uuid() + "." + kAIChatLeoWorkspaceUIHost, url.host());
  EXPECT_EQ("/", url.path());

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
  auto content = CreateContent(CreateWorkspaceFolder());
  const GURL workspace_url = content->url();
  ASSERT_EQ(
      CONTENT_SETTING_ASK,
      GetSetting(workspace_url, ContentSettingsType::FILE_SYSTEM_READ_GUARD));
  ASSERT_EQ(
      CONTENT_SETTING_ASK,
      GetSetting(workspace_url, ContentSettingsType::FILE_SYSTEM_WRITE_GUARD));

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

  // The grant is scoped to this workspace's own origin: it must not extend to
  // the workspace host itself, nor to any other workspace's subdomain.
  for (const GURL& other_url :
       {GURL(base::StrCat({content::kChromeUIUntrustedScheme,
                           url::kStandardSchemeSeparator,
                           kAIChatLeoWorkspaceUIHost, "/"})),
        GURL(base::StrCat({content::kChromeUIUntrustedScheme,
                           url::kStandardSchemeSeparator, "other",
                           kAIChatLeoWorkspaceUIHostSuffix, "/"}))}) {
    EXPECT_EQ(
        CONTENT_SETTING_ASK,
        GetSetting(other_url, ContentSettingsType::FILE_SYSTEM_READ_GUARD))
        << other_url;
    EXPECT_EQ(
        CONTENT_SETTING_ASK,
        GetSetting(other_url, ContentSettingsType::FILE_SYSTEM_WRITE_GUARD))
        << other_url;
  }
}

// Two workspaces must end up as two entirely separate origins, in separate
// renderer processes, which is the point of the per-workspace subdomain.
IN_PROC_BROWSER_TEST_F(WorkspaceAssociatedContentBrowserTest,
                       WorkspacesAreIsolatedFromEachOther) {
  auto first = CreateContent(CreateWorkspaceFolder());
  auto second = CreateContent(CreateWorkspaceFolder());

  content::WebContents* first_contents = first->GetWebContentsForTesting();
  content::WebContents* second_contents = second->GetWebContentsForTesting();
  ASSERT_TRUE(content::WaitForLoadStop(first_contents));
  ASSERT_TRUE(content::WaitForLoadStop(second_contents));

  const url::Origin first_origin =
      first_contents->GetPrimaryMainFrame()->GetLastCommittedOrigin();
  const url::Origin second_origin =
      second_contents->GetPrimaryMainFrame()->GetLastCommittedOrigin();
  ASSERT_FALSE(first_origin.opaque());
  ASSERT_FALSE(second_origin.opaque());
  EXPECT_EQ(url::Origin::Create(first->url()), first_origin);
  EXPECT_EQ(url::Origin::Create(second->url()), second_origin);
  EXPECT_NE(first_origin, second_origin);

  // Distinct sites, so distinct processes.
  EXPECT_NE(first_contents->GetPrimaryMainFrame()->GetProcess(),
            second_contents->GetPrimaryMainFrame()->GetProcess());

  // Each subdomain is served its own bundle, rather than falling back to a data
  // source registered for the workspace host.
  EXPECT_TRUE(first_contents->GetWebUI());
  EXPECT_TRUE(second_contents->GetWebUI());
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

// A workspace displays its contents in an iframe served from a further
// subdomain of its own host, so the viewer is a separate origin holding none of
// the workspace's File System Access grants and none of its storage.
IN_PROC_BROWSER_TEST_F(WorkspaceAssociatedContentBrowserTest,
                       WorkspaceFramesItsOwnViewer) {
  auto content = CreateContent(CreateWorkspaceFolder());
  content::WebContents* web_contents = content->GetWebContentsForTesting();
  ASSERT_TRUE(content::WaitForLoadStop(web_contents));

  const GURL viewer_url = ViewerURL(content->url());
  ASSERT_EQ(base::StrCat({kAIChatLeoWorkspaceViewUIHostPrefix, content->uuid(),
                          kAIChatLeoWorkspaceUIHostSuffix}),
            viewer_url.host());

  ASSERT_TRUE(FrameLoads(web_contents, viewer_url));

  content::RenderFrameHost* viewer =
      content::ChildFrameAt(web_contents->GetPrimaryMainFrame(), 0);
  ASSERT_TRUE(viewer);
  EXPECT_EQ(url::Origin::Create(viewer_url), viewer->GetLastCommittedOrigin());
  EXPECT_NE(web_contents->GetPrimaryMainFrame()->GetLastCommittedOrigin(),
            viewer->GetLastCommittedOrigin());

  // Grants are per-origin, and only the workspace is granted one.
  EXPECT_EQ(
      CONTENT_SETTING_ASK,
      GetSetting(viewer_url, ContentSettingsType::FILE_SYSTEM_READ_GUARD));
  EXPECT_EQ(
      CONTENT_SETTING_ASK,
      GetSetting(viewer_url, ContentSettingsType::FILE_SYSTEM_WRITE_GUARD));
}

// Framing another workspace's viewer would let a workspace present unrelated
// contents as its own. The framer's frame-src rejects it first, so this only
// pins the outcome; the viewer's frame-ancestors is the redundant second half
// and no origin in this feature can reach it to exercise it on its own.
IN_PROC_BROWSER_TEST_F(WorkspaceAssociatedContentBrowserTest,
                       WorkspaceCannotFrameAnotherWorkspacesViewer) {
  auto first = CreateContent(CreateWorkspaceFolder());
  auto second = CreateContent(CreateWorkspaceFolder());
  content::WebContents* first_contents = first->GetWebContentsForTesting();
  ASSERT_TRUE(content::WaitForLoadStop(first_contents));
  ASSERT_TRUE(content::WaitForLoadStop(second->GetWebContentsForTesting()));

  ASSERT_TRUE(FrameLoads(first_contents, ViewerURL(first->url())));
  EXPECT_FALSE(FrameLoads(first_contents, ViewerURL(second->url())));
  EXPECT_FALSE(first_contents->IsCrashed());
}

// Opting into subdomains hands the workspace config every otherwise unclaimed
// host under its own, so it has to turn down the shapes it does not serve
// rather than serving a workspace for them.
IN_PROC_BROWSER_TEST_F(WorkspaceAssociatedContentBrowserTest,
                       UnrecognizedWorkspaceHostsDoNotLoad) {
  const std::string prefix = base::StrCat(
      {content::kChromeUIUntrustedScheme, url::kStandardSchemeSeparator});
  for (const GURL& url : {
           // The registered host belongs to no workspace.
           GURL(base::StrCat({prefix, kAIChatLeoWorkspaceUIHost})),
           // Too deep to be a workspace, and no viewer prefix.
           GURL(base::StrCat({prefix, "a.b", kAIChatLeoWorkspaceUIHostSuffix})),
           // A viewer belongs to exactly one workspace, at a fixed depth.
           GURL(base::StrCat({prefix, kAIChatLeoWorkspaceViewUIHostPrefix,
                              "a.b", kAIChatLeoWorkspaceUIHostSuffix})),
           GURL(base::StrCat({prefix, kAIChatLeoWorkspaceViewUIHostPrefix,
                              kAIChatLeoWorkspaceViewUIHostPrefix, "abc",
                              kAIChatLeoWorkspaceUIHostSuffix})),
       }) {
    EXPECT_FALSE(NavigateActiveTabAndGetSuccess(url)) << url;
    content::WebContents* web_contents =
        browser()->tab_strip_model()->GetActiveWebContents();
    EXPECT_FALSE(web_contents->IsCrashed()) << url;
    EXPECT_FALSE(web_contents->GetWebUI()) << url;
  }
}

// A workspace's label is an opaque id, so it is the host's shape that makes it
// a workspace's, never what the label reads as: the host that looks like a
// viewer of the workspace host is just a workspace whose id happens to be
// "view".
IN_PROC_BROWSER_TEST_F(WorkspaceAssociatedContentBrowserTest,
                       WorkspaceIdIsNotInterpreted) {
  const GURL url(base::StrCat(
      {content::kChromeUIUntrustedScheme, url::kStandardSchemeSeparator,
       kAIChatLeoWorkspaceViewUIHostPrefix, kAIChatLeoWorkspaceUIHost}));
  EXPECT_TRUE(NavigateActiveTabAndGetSuccess(url)) << url;
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  EXPECT_TRUE(content::WaitForLoadStop(web_contents));
  // The host is a workspace (`view.` is its id), so it is served the
  // workspace bundle, not the viewer one.
  EXPECT_EQ(u"Leo Workspace", web_contents->GetTitle());
}

// Fixture for the page-side half of the pipeline, which needs WebMCP so that
// the workspace page can register its tools.
class WorkspaceAssociatedContentWebMcpBrowserTest
    : public WorkspaceAssociatedContentBrowserTest {
 public:
  WorkspaceAssociatedContentWebMcpBrowserTest() {
    web_mcp_feature_list_.InitAndEnableFeature(blink::features::kWebMCP);
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    WorkspaceAssociatedContentBrowserTest::SetUpCommandLine(command_line);
    // The runtime-enabled feature gating document.modelContext is marked
    // "experimental", so the base::Feature toggle alone isn't enough to turn it
    // on in the renderer.
    command_line->AppendSwitchASCII("enable-blink-features", "WebMCP");
  }

 private:
  base::test::ScopedFeatureList web_mcp_feature_list_;
};

// The workspace page registers its file tools via WebMCP, which blink only
// permits for workspace documents. That check is on the host, so it has to
// accept the per-workspace subdomain the page is actually served from.
IN_PROC_BROWSER_TEST_F(WorkspaceAssociatedContentWebMcpBrowserTest,
                       PageCanRegisterToolsFromItsOwnSubdomain) {
  auto content = CreateContent(CreateWorkspaceFolder());
  content::WebContents* web_contents = content->GetWebContentsForTesting();
  ASSERT_TRUE(content::WaitForLoadStop(web_contents));
  ASSERT_EQ(content->url(), web_contents->GetLastCommittedURL());

  // registerTool() rejects with a SecurityError when WebMCP isn't allowed for
  // the document's origin, so the promise resolving is the assertion here.
  EXPECT_EQ("registered", content::EvalJs(web_contents, R"JS(
      (async () => {
        await document.modelContext.registerTool({
          name: 'test_tool',
          description: 'A tool registered by the test',
          execute: async () => 'ok',
        });
        return 'registered';
      })()
  )JS"));
}

// The viewer has no tools of its own, so blink's WebMCP gate must not extend to
// it just because its host ends with the workspace host.
IN_PROC_BROWSER_TEST_F(WorkspaceAssociatedContentWebMcpBrowserTest,
                       ViewerCannotRegisterTools) {
  auto content = CreateContent(CreateWorkspaceFolder());
  content::WebContents* web_contents = content->GetWebContentsForTesting();
  ASSERT_TRUE(content::WaitForLoadStop(web_contents));
  ASSERT_TRUE(FrameLoads(web_contents, ViewerURL(content->url())));

  content::RenderFrameHost* viewer =
      content::ChildFrameAt(web_contents->GetPrimaryMainFrame(), 0);
  ASSERT_TRUE(viewer);
  EXPECT_EQ("SecurityError", content::EvalJs(viewer, R"JS(
      (async () => {
        try {
          await document.modelContext.registerTool({
            name: 'test_tool',
            description: 'A tool the viewer must not be able to register',
            execute: async () => 'ok',
          });
          return 'registered';
        } catch (error) {
          return error.name;
        }
      })()
  )JS"));
}

}  // namespace ai_chat
