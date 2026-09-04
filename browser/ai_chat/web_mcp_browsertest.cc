// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <set>
#include <string>
#include <string_view>

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/ai_chat_service_factory.h"
#include "brave/components/ai_chat/content/browser/ai_chat_tab_helper.h"
#include "brave/components/ai_chat/content/browser/associated_web_contents_content.h"
#include "brave/components/ai_chat/core/browser/ai_chat_service.h"
#include "brave/components/ai_chat/core/browser/associated_content_delegate.h"
#include "brave/components/ai_chat/core/browser/associated_content_manager.h"
#include "brave/components/ai_chat/core/browser/conversation_handler.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_mock_cert_verifier.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/features.h"

namespace ai_chat {

namespace {

// Test pages served from test/data/leo/. The tool-registering page uses
// document.modelContext, which is only available on secure contexts when
// blink::features::kWebMCP is enabled.
constexpr char kPageWithToolsPath[] = "/web_mcp_tools.html";
// A basic existing page that registers no tools.
constexpr char kPageWithoutToolsPath[] = "/dummy.html";

}  // namespace

class WebMcpBrowserTest : public InProcessBrowserTest {
 public:
  WebMcpBrowserTest() : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    // document.modelContext is gated by kWebMCP. The base::Feature flips on the
    // runtime-enabled feature in every renderer the browser spawns during this
    // test.
    scoped_feature_list_.InitWithFeatures(
        /*enabled_features=*/{blink::features::kWebMCP},
        /*disabled_features=*/{});
  }

  ~WebMcpBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    mock_cert_verifier_.mock_cert_verifier()->set_default_result(net::OK);
    host_resolver()->AddRule("*", "127.0.0.1");

    base::FilePath test_data_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);
    https_server_.ServeFilesFromDirectory(test_data_dir.AppendASCII("leo"));
    ASSERT_TRUE(https_server_.Start());
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    mock_cert_verifier_.SetUpCommandLine(command_line);
    // The runtime-enabled feature gating document.modelContext is marked
    // "experimental" in runtime_enabled_features.json5 so the base::Feature
    // toggle alone isn't enough; the blink-feature switch turns it on in
    // every renderer for the duration of this test.
    command_line->AppendSwitchASCII("enable-blink-features", "WebMCP");
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
    mock_cert_verifier_.SetUpInProcessBrowserTestFixture();
  }

  void TearDownInProcessBrowserTestFixture() override {
    mock_cert_verifier_.TearDownInProcessBrowserTestFixture();
    InProcessBrowserTest::TearDownInProcessBrowserTestFixture();
  }

 protected:
  content::WebContents* GetActiveWebContents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  AssociatedWebContentsContent* GetActiveContent() {
    auto* helper = AIChatTabHelper::FromWebContents(GetActiveWebContents());
    return helper ? &helper->web_contents_content() : nullptr;
  }

  // Blocks until the tool-registering page has finished its async
  // registerTool() calls, and evaluates to the number of tools the page itself
  // sees via document.modelContext.getTools(). Callers should check this before
  // probing the browser side, both to avoid racing registration and so that a
  // page-side failure is reported here (as a promise rejection) rather than
  // showing up as a confusing "zero tools" further down.
  content::EvalJsResult WaitForPageToolCount() {
    return content::EvalJs(GetActiveWebContents(), "__webmcpReady");
  }

  content::EvalJsResult GetPageToolCount() {
    return content::EvalJs(
        GetActiveWebContents(),
        "(async () => (await document.modelContext.getTools()).length)()");
  }

  // Stashes each tool's AbortController so UnregisterPageTools() can
  // unregister them later.
  content::EvalJsResult RegisterPageTool(std::string_view name) {
    return content::EvalJs(GetActiveWebContents(), content::JsReplace(R"JS(
      (async () => {
        window.__toolControllers = window.__toolControllers ?? [];
        const controller = new AbortController();
        window.__toolControllers.push(controller);
        await document.modelContext.registerTool({
          name: $1,
          description: "A tool registered by the test",
          execute: async () => "ok",
        }, { signal: controller.signal });
        return (await document.modelContext.getTools()).length;
      })()
    )JS",
                                                                      name));
  }

  [[nodiscard]] bool UnregisterPageTools() {
    return content::ExecJs(GetActiveWebContents(),
                           "window.__toolControllers.forEach(c => c.abort())");
  }

  void ReloadActiveTab() {
    auto* web_contents = GetActiveWebContents();
    content::WaitForLoadStop(web_contents);
    web_contents->GetController().Reload(content::ReloadType::NORMAL,
                                         /*check_for_repost=*/false);
    content::WaitForLoadStop(web_contents);
  }

  AssociatedContentManager* CreateConversationManager() {
    auto* conversation =
        AIChatServiceFactory::GetForBrowserContext(browser()->GetProfile())
            ->CreateConversation();
    EXPECT_TRUE(conversation);
    return conversation->associated_content_manager();
  }

  // Drives an empty new-generation-loop on the manager so it re-fetches the
  // current set of tools, then returns them.
  std::vector<base::WeakPtr<Tool>> RefreshAndGetTools(
      AssociatedContentManager* manager) {
    base::test::TestFuture<void> done;
    manager->UpdateToolsForNewGenerationLoop(done.GetCallback());
    EXPECT_TRUE(done.Wait());
    return manager->GetTools();
  }

  net::EmbeddedTestServer* https_server() { return &https_server_; }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  content::ContentMockCertVerifier mock_cert_verifier_;
  net::EmbeddedTestServer https_server_;
};

// Registering tools via document.modelContext on the active page should be
// observable through AssociatedContentManager::GetTools() once a generation
// loop runs.
// Diagnostic: confirms the renderer-side path returns script tools to brave's
// content delegate, independent of the AssociatedContentManager wiring.
IN_PROC_BROWSER_TEST_F(WebMcpBrowserTest,
                       AssociatedWebContentsContent_DirectFetch) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("a.com", kPageWithToolsPath)));
  ASSERT_EQ(2, WaitForPageToolCount());

  AssociatedContentDelegate* content = GetActiveContent();
  ASSERT_TRUE(content);

  base::test::TestFuture<std::vector<std::unique_ptr<Tool>>> future;
  content->GetContentTools(future.GetCallback());
  EXPECT_EQ(2u, future.Take().size());
}

IN_PROC_BROWSER_TEST_F(WebMcpBrowserTest,
                       AssociatedContentManager_SeesRegisteredTools) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("a.com", kPageWithToolsPath)));

  // Confirm the page-side registration completed before probing the browser
  // side; without this the rest of the test would just see zero tools and the
  // failure would be hard to diagnose.
  ASSERT_EQ(2, WaitForPageToolCount());

  AssociatedContentDelegate* content = GetActiveContent();
  ASSERT_TRUE(content);

  auto* manager = CreateConversationManager();
  manager->AddContent(content);

  // AddContent attaches content that exposes tools, but the detection probe is
  // an async round-trip to the renderer. The generation loop only fetches tools
  // from attached content, so wait for detection to complete first.
  ASSERT_TRUE(base::test::RunUntil([&] { return content->tools_attached(); }));

  auto tools = RefreshAndGetTools(manager);
  ASSERT_EQ(2u, tools.size());

  // Names are prefixed with "web_" and the sanitized host ("a.com" → "a_com").
  // Only the host is used (not the full path) to keep the name within Bedrock's
  // 64-char tool-name limit.
  std::set<std::string> tool_names;
  for (const auto& tool : tools) {
    ASSERT_TRUE(tool);
    tool_names.insert(std::string(tool->Name()));
  }
  EXPECT_THAT(tool_names, ::testing::UnorderedElementsAre("web_a_com_echo",
                                                          "web_a_com_ping"));

  // Sanity check on metadata for the richer tool.
  for (const auto& tool : tools) {
    if (tool->Name() == "web_a_com_echo") {
      // The description embeds the full page URL and the page-provided
      // description.
      std::string description(tool->Description());
      EXPECT_NE(description.find("a.com"), std::string::npos);
      EXPECT_NE(description.find(kPageWithToolsPath), std::string::npos);
      EXPECT_NE(description.find("Echo input back"), std::string::npos);
      ASSERT_TRUE(tool->InputProperties().has_value());
      EXPECT_TRUE(tool->InputProperties()->contains("text"));
      ASSERT_TRUE(tool->RequiredProperties().has_value());
      EXPECT_THAT(*tool->RequiredProperties(), ::testing::ElementsAre("text"));
    }
  }
}

// A page with no script tools must not synthesize any.
IN_PROC_BROWSER_TEST_F(WebMcpBrowserTest,
                       AssociatedContentManager_NoToolsWhenPageHasNone) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("a.com", kPageWithoutToolsPath)));

  auto* content = GetActiveContent();
  ASSERT_TRUE(content);

  auto* manager = CreateConversationManager();
  manager->AddContent(content);

  EXPECT_TRUE(RefreshAndGetTools(manager).empty());
}

// Re-running the generation loop after a navigation should pick up the new
// page's tools and drop the old page's tools.
IN_PROC_BROWSER_TEST_F(WebMcpBrowserTest,
                       AssociatedContentManager_RefreshesAcrossNavigations) {
  auto* manager = CreateConversationManager();

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("a.com", kPageWithToolsPath)));
  ASSERT_EQ(2, WaitForPageToolCount());
  auto* content = GetActiveContent();
  manager->AddContent(content);
  // Wait for the async tool-detection probe to attach the content before the
  // generation loop, which only fetches tools from attached content.
  ASSERT_TRUE(base::test::RunUntil([&] { return content->tools_attached(); }));
  EXPECT_EQ(2u, RefreshAndGetTools(manager).size());

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("a.com", kPageWithoutToolsPath)));
  EXPECT_TRUE(RefreshAndGetTools(manager).empty());
}

// Late-registered tools attach the content via the `toolchange` notification.
IN_PROC_BROWSER_TEST_F(WebMcpBrowserTest,
                       AssociatedContentManager_AttachesOnLateRegistration) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("a.com", kPageWithoutToolsPath)));

  auto* content = GetActiveContent();
  ASSERT_TRUE(content);

  auto* manager = CreateConversationManager();
  manager->AddContent(content);
  ASSERT_FALSE(content->tools_attached());

  ASSERT_EQ(1, RegisterPageTool("late_tool"));

  ASSERT_TRUE(base::test::RunUntil([&] { return content->tools_attached(); }));
  EXPECT_EQ(1u, RefreshAndGetTools(manager).size());
}

// Unregistering all of the page's tools detaches the staged content again.
IN_PROC_BROWSER_TEST_F(WebMcpBrowserTest,
                       AssociatedContentManager_DetachesOnUnregistration) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("a.com", kPageWithoutToolsPath)));

  auto* content = GetActiveContent();
  ASSERT_TRUE(content);

  auto* manager = CreateConversationManager();
  manager->AddContent(content);
  ASSERT_FALSE(content->tools_attached());

  ASSERT_EQ(1, RegisterPageTool("tool_one"));
  ASSERT_EQ(2, RegisterPageTool("tool_two"));
  ASSERT_TRUE(base::test::RunUntil([&] { return content->tools_attached(); }));

  ASSERT_TRUE(UnregisterPageTools());
  // Confirm the page saw the unregistration, so a failure below is
  // browser-side.
  ASSERT_EQ(0, GetPageToolCount());

  ASSERT_TRUE(base::test::RunUntil([&] { return !content->tools_attached(); }));
  EXPECT_TRUE(RefreshAndGetTools(manager).empty());
}

// A reload re-establishes the subscription, so late-registered tools still
// attach the content.
IN_PROC_BROWSER_TEST_F(
    WebMcpBrowserTest,
    AssociatedContentManager_AttachesOnRegistrationAfterReload) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("a.com", kPageWithoutToolsPath)));

  auto* manager = CreateConversationManager();
  auto* content = GetActiveContent();
  ASSERT_TRUE(content);
  manager->AddContent(content);
  ASSERT_FALSE(content->tools_attached());

  ReloadActiveTab();
  ASSERT_FALSE(content->tools_attached());

  ASSERT_EQ(1, RegisterPageTool("late_tool"));

  ASSERT_TRUE(base::test::RunUntil([&] { return content->tools_attached(); }));
  EXPECT_EQ(1u, RefreshAndGetTools(manager).size());
}

// A reload keeps the content associated: the live delegate keeps its uuid, so
// the page's re-registered tools re-attach without the frontend re-attaching.
IN_PROC_BROWSER_TEST_F(WebMcpBrowserTest,
                       AssociatedContentManager_ReloadKeepsAssociation) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("a.com", kPageWithToolsPath)));
  ASSERT_EQ(2, WaitForPageToolCount());

  auto* manager = CreateConversationManager();
  auto* content = GetActiveContent();
  ASSERT_TRUE(content);
  manager->AddContent(content);
  ASSERT_TRUE(base::test::RunUntil([&] { return content->tools_attached(); }));

  auto associated = manager->GetAssociatedContent();
  ASSERT_EQ(1u, associated.size());
  const std::string uuid_before = associated[0]->uuid;

  ReloadActiveTab();
  ASSERT_EQ(2, WaitForPageToolCount());

  associated = manager->GetAssociatedContent();
  ASSERT_EQ(1u, associated.size());
  EXPECT_EQ(uuid_before, associated[0]->uuid);
  EXPECT_EQ("WebMCP test", associated[0]->title);
  ASSERT_TRUE(base::test::RunUntil([&] { return content->tools_attached(); }));
  EXPECT_EQ(2u, RefreshAndGetTools(manager).size());
}

// A renderer-initiated reload is a converted reload, so the entry gets a new
// UniqueID, which the delegate must track as the frontend's content_id.
IN_PROC_BROWSER_TEST_F(
    WebMcpBrowserTest,
    AssociatedContentManager_LocationReloadKeepsAssociation) {
  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("a.com", kPageWithToolsPath)));
  ASSERT_EQ(2, WaitForPageToolCount());

  auto* manager = CreateConversationManager();
  auto* content = GetActiveContent();
  ASSERT_TRUE(content);
  manager->AddContent(content);
  ASSERT_TRUE(base::test::RunUntil([&] { return content->tools_attached(); }));

  auto associated = manager->GetAssociatedContent();
  ASSERT_EQ(1u, associated.size());
  const std::string uuid_before = associated[0]->uuid;

  auto* web_contents = GetActiveWebContents();
  content::WaitForLoadStop(web_contents);
  ASSERT_TRUE(content::ExecJs(web_contents, "location.reload()"));
  content::WaitForLoadStop(web_contents);
  ASSERT_EQ(2, WaitForPageToolCount());

  associated = manager->GetAssociatedContent();
  ASSERT_EQ(1u, associated.size());
  EXPECT_EQ(uuid_before, associated[0]->uuid);
  EXPECT_EQ("WebMCP test", associated[0]->title);
  EXPECT_EQ(
      web_contents->GetController().GetLastCommittedEntry()->GetUniqueID(),
      associated[0]->content_id);

  ASSERT_TRUE(base::test::RunUntil([&] { return content->tools_attached(); }));
  EXPECT_EQ(2u, RefreshAndGetTools(manager).size());
}

}  // namespace ai_chat
