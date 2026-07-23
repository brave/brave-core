// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <set>
#include <string>

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
// blink::features::kWebMCPTesting is enabled (which implies kWebMCP).
constexpr char kPageWithToolsPath[] = "/web_mcp_tools.html";
// A basic existing page that registers no tools.
constexpr char kPageWithoutToolsPath[] = "/dummy.html";

}  // namespace

class WebMcpBrowserTest : public InProcessBrowserTest {
 public:
  WebMcpBrowserTest() : https_server_(net::EmbeddedTestServer::TYPE_HTTPS) {
    // document.modelContext is gated by kWebMCPTesting (which implies kWebMCP).
    // The base::Feature flips on the runtime-enabled feature in every renderer
    // the browser spawns during this test.
    scoped_feature_list_.InitWithFeatures(
        /*enabled_features=*/{blink::features::kWebMCPTesting},
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
    // toggle alone isn't enough; the blink-feature switch turns it on in every
    // renderer for the duration of this test.
    command_line->AppendSwitchASCII("enable-blink-features", "WebMCPTesting");
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
  ASSERT_EQ(2, content::EvalJs(
                   GetActiveWebContents(),
                   "document.modelContext.getTools().then(t => t.length)"));

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

  // Make sure the WebMCP runtime feature is actually present in the renderer
  // and the page-side registration didn't throw; without this the rest of the
  // test would just see zero tools and the failure would be hard to diagnose.
  EXPECT_EQ(true, content::EvalJs(GetActiveWebContents(),
                                  "typeof document.modelContext === 'object'"));
  EXPECT_EQ("null", content::EvalJs(GetActiveWebContents(),
                                    "String(window.__webmcpRegisterError)"));
  // Sanity check directly with the renderer that the tools are registered on
  // document.modelContext at the moment the manager will fetch them.
  ASSERT_EQ(2, content::EvalJs(
                   GetActiveWebContents(),
                   "document.modelContext.getTools().then(t => t.length)"));

  AssociatedContentDelegate* content = GetActiveContent();
  ASSERT_TRUE(content);

  auto* ai_chat_service =
      AIChatServiceFactory::GetForBrowserContext(browser()->profile());
  auto* conversation = ai_chat_service->CreateConversation();
  ASSERT_TRUE(conversation);

  auto* manager = conversation->associated_content_manager();
  manager->AddContent(content);

  // AddContent attaches content that exposes tools, but the detection probe is
  // an async round-trip to the renderer. The generation loop only fetches tools
  // from attached content, so wait for detection to complete first.
  ASSERT_TRUE(base::test::RunUntil([&] { return content->tools_attached(); }));

  auto tools = RefreshAndGetTools(manager);
  ASSERT_EQ(2u, tools.size());

  // Names are prefixed with the sanitized host and path
  // ("a.com/web_mcp_tools.html" → "a_com_web_mcp_tools_html") to disambiguate
  // page-defined tools across origins and pages.
  std::set<std::string> tool_names;
  for (const auto& tool : tools) {
    ASSERT_TRUE(tool);
    tool_names.insert(std::string(tool->Name()));
  }
  EXPECT_THAT(tool_names,
              ::testing::UnorderedElementsAre("a_com_web_mcp_tools_html_echo",
                                              "a_com_web_mcp_tools_html_ping"));

  // Sanity check on metadata for the richer tool.
  for (const auto& tool : tools) {
    if (tool->Name() == "a_com_web_mcp_tools_html_echo") {
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

  auto* ai_chat_service =
      AIChatServiceFactory::GetForBrowserContext(browser()->profile());
  auto* conversation = ai_chat_service->CreateConversation();
  ASSERT_TRUE(conversation);

  auto* manager = conversation->associated_content_manager();
  manager->AddContent(content);

  EXPECT_TRUE(RefreshAndGetTools(manager).empty());
}

// Re-running the generation loop after a navigation should pick up the new
// page's tools and drop the old page's tools.
IN_PROC_BROWSER_TEST_F(WebMcpBrowserTest,
                       AssociatedContentManager_RefreshesAcrossNavigations) {
  auto* ai_chat_service =
      AIChatServiceFactory::GetForBrowserContext(browser()->profile());
  auto* conversation = ai_chat_service->CreateConversation();
  ASSERT_TRUE(conversation);
  auto* manager = conversation->associated_content_manager();

  ASSERT_TRUE(ui_test_utils::NavigateToURL(
      browser(), https_server()->GetURL("a.com", kPageWithToolsPath)));
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

}  // namespace ai_chat
