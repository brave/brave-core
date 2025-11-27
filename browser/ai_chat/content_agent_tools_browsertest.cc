// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>
#include <vector>

#include "base/json/json_writer.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/browser/ai_chat/ai_chat_agent_profile_helper.h"
#include "brave/browser/ai_chat/content_agent_tool_provider.h"
#include "brave/browser/ai_chat/tools/target_test_util.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "chrome/browser/actor/actor_keyed_service_factory.h"
#include "chrome/browser/actor/actor_policy_checker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

namespace {

// Substring that indicates a successful tool result
const char kToolResultSuccessSubstring[] = "successful";

}  // namespace

// These tests verify, end to end, that the various content tools utilize
// the Chromium actor framework successfully. They do not need to test all
// edge cases with either the actor framework or the tool param parsing, since
// that is covered by the chromium actor browser tests and the brave tool unit
// tests. They are largely duplicates of the most simple test case in each
// tool's Chromium actor browser test.
class ContentAgentToolsTest : public InProcessBrowserTest {
 public:
  ContentAgentToolsTest() {
    scoped_feature_list_.InitAndEnableFeature(
        ai_chat::features::kAIChatAgentProfile);
  }

  ~ContentAgentToolsTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());
    ASSERT_TRUE(embedded_https_test_server().Start());

    // Create the agent profile
    auto* profile = browser()->profile();
    SetUserOptedIn(profile->GetPrefs(), true);
    base::test::TestFuture<Browser*> browser_future;
    OpenBrowserWindowForAIChatAgentProfileForTesting(
        *profile, browser_future.GetCallback());
    Browser* browser = browser_future.Take();
    ASSERT_NE(browser, nullptr);
    agent_profile_ = browser->profile();

    // Get the actor service
    auto* actor_service =
        actor::ActorKeyedServiceFactory::GetActorKeyedService(GetProfile());
    ASSERT_NE(actor_service, nullptr);

    // Get the browser tool provider
    tool_provider_ =
        std::make_unique<ContentAgentToolProvider>(GetProfile(), actor_service);
    ASSERT_NE(tool_provider_, nullptr);
  }

  void TearDownOnMainThread() override {
    tool_provider_.reset();
    agent_profile_ = nullptr;
    InProcessBrowserTest::TearDownOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    // Ensure physical and css pixels are the same, as per tools_test_util.cc
    // - simplifies scroll distance calculations.
    command_line->AppendSwitchASCII(switches::kForceDeviceScaleFactor, "1");
  }

 protected:
  // Helper to find a tool by name
  base::WeakPtr<Tool> FindToolByName(const std::string& name) {
    auto tools = tool_provider_->GetTools();
    for (auto& tool : tools) {
      if (tool && tool->Name() == name) {
        return tool;
      }
    }
    return nullptr;
  }

  // Helper to execute a tool and wait for completion
  Tool::ToolResult ExecuteToolAndWait(base::WeakPtr<Tool> tool,
                                      const std::string& input_json,
                                      bool verify_success = true) {
    base::test::TestFuture<Tool::ToolResult> result_future;
    tool->UseTool(input_json, result_future.GetCallback());
    auto result = result_future.Take();
    if (verify_success) {
      EXPECT_THAT(result, ContentBlockText(
                              testing::HasSubstr(kToolResultSuccessSubstring)));
    }
    return result;
  }

  // Helper to get the document identifier for the main frame
  std::string GetMainFrameDocumentIdentifier() {
    auto document_identifier =
        optimization_guide::DocumentIdentifierUserData::GetDocumentIdentifier(
            web_contents()->GetPrimaryMainFrame()->GetGlobalFrameToken());
    CHECK(document_identifier.has_value());
    return document_identifier.value();
  }

  // Helper to get real DOM node ID from a page element
  int GetDOMNodeId(const std::string& selector) {
    return content::GetDOMNodeId(*main_frame(), selector).value();
  }

  // Helper to get the web contents
  content::WebContents* web_contents() {
    base::test::TestFuture<tabs::TabHandle> tab_handle_future;
    tool_provider_->GetOrCreateTabHandleForTask(
        tab_handle_future.GetCallback());
    tabs::TabHandle tab_handle = tab_handle_future.Take();
    return tab_handle.Get()->GetContents();
  }

  // Helper to get the main frame
  content::RenderFrameHost* main_frame() {
    return web_contents()->GetPrimaryMainFrame();
  }

  // Helper to get the profile
  Profile* GetProfile() { return agent_profile_; }

  // Helper to navigate to Chromium test files
  void NavigateToChromiumTestFile(const std::string& file_path) {
    base::test::TestFuture<tabs::TabHandle> tab_handle_future;
    tool_provider_->GetOrCreateTabHandleForTask(
        tab_handle_future.GetCallback());
    tabs::TabHandle tab_handle = tab_handle_future.Take();

    GURL test_url = embedded_test_server()->GetURL(file_path);
    ASSERT_TRUE(
        content::NavigateToURL(tab_handle.Get()->GetContents(), test_url));
  }

  raw_ptr<Profile> agent_profile_;
  std::unique_ptr<ContentAgentToolProvider> tool_provider_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

// Test click tool with Node ID targeting
IN_PROC_BROWSER_TEST_F(ContentAgentToolsTest, ClickTool_NodeIdTarget) {
  NavigateToChromiumTestFile("/actor/page_with_clickable_element.html");

  auto click_tool = FindToolByName("click_element");
  ASSERT_TRUE(click_tool);

  // Get real DOM node ID for the clickable button
  int button_node_id = GetDOMNodeId("button#clickable");

  auto target_dict = target_test_util::GetContentNodeTargetDict(
      button_node_id, GetMainFrameDocumentIdentifier());

  base::Value::Dict input;
  input.Set("target", target_dict.Clone());
  input.Set("click_type", "left");
  input.Set("click_count", "single");

  auto result = ExecuteToolAndWait(click_tool, *base::WriteJson(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the button was actually clicked
  EXPECT_EQ(true, content::EvalJs(web_contents(), "button_clicked"));

  // Verify mouse events were fired
  std::string mouse_events =
      content::EvalJs(web_contents(), "mouse_event_log.join(',')")
          .ExtractString();
  EXPECT_TRUE(mouse_events.find("click[BUTTON#clickable]") !=
              std::string::npos);
}

// Test type tool with Node ID targeting
IN_PROC_BROWSER_TEST_F(ContentAgentToolsTest, TypeTool_NodeIdTarget) {
  NavigateToChromiumTestFile("/actor/input.html");

  auto type_tool = FindToolByName("type_text");
  ASSERT_TRUE(type_tool);

  // Get real DOM node ID for the input element
  int input_node_id = GetDOMNodeId("#input");
  auto target_dict = target_test_util::GetContentNodeTargetDict(
      input_node_id, GetMainFrameDocumentIdentifier());

  base::Value::Dict input;
  input.Set("target", target_dict.Clone());
  input.Set("text", "Hello World");
  input.Set("follow_by_enter", false);
  input.Set("mode", "replace");

  auto result = ExecuteToolAndWait(type_tool, *base::WriteJson(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the text content was actually entered
  std::string input_value =
      content::EvalJs(web_contents(), "document.getElementById('input').value")
          .ExtractString();
  EXPECT_EQ("Hello World", input_value);
}

// Test scroll tool with Node ID targeting
IN_PROC_BROWSER_TEST_F(ContentAgentToolsTest, ScrollTool_NodeIdTarget) {
  NavigateToChromiumTestFile("/actor/scrollable_page.html");

  auto scroll_tool = FindToolByName("scroll_element");
  ASSERT_TRUE(scroll_tool);

  // Get initial scroll position of the scroller element
  int initial_scroll =
      content::EvalJs(web_contents(),
                      "document.getElementById('scroller').scrollTop")
          .ExtractInt();

  // Get real DOM node ID for the scroller element
  int scroller_node_id = GetDOMNodeId("#scroller");
  auto target_dict = target_test_util::GetContentNodeTargetDict(
      scroller_node_id, GetMainFrameDocumentIdentifier());

  base::Value::Dict input;
  input.Set("target", target_dict.Clone());
  input.Set("direction", "down");
  input.Set("distance", 50);

  auto result = ExecuteToolAndWait(scroll_tool, *base::WriteJson(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the element was scrolled down
  int final_scroll =
      content::EvalJs(web_contents(),
                      "document.getElementById('scroller').scrollTop")
          .ExtractInt();
  EXPECT_GT(final_scroll, initial_scroll);
  EXPECT_EQ(final_scroll, 50);
}

// Test scroll tool with Node ID targeting
IN_PROC_BROWSER_TEST_F(ContentAgentToolsTest, ScrollTool_DocumentTarget) {
  NavigateToChromiumTestFile("/actor/scrollable_page.html");

  auto scroll_tool = FindToolByName("scroll_element");
  ASSERT_TRUE(scroll_tool);

  int scroll_distance = 50;

  ASSERT_EQ(0, EvalJs(web_contents(), "window.scrollY"));

  auto target_dict =
      target_test_util::GetDocumentTargetDict(GetMainFrameDocumentIdentifier());

  base::Value::Dict input;
  input.Set("target", target_dict.Clone());
  input.Set("direction", "down");
  input.Set("distance", scroll_distance);

  auto result = ExecuteToolAndWait(scroll_tool, *base::WriteJson(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the element was scrolled down
  EXPECT_EQ(scroll_distance, EvalJs(web_contents(), "window.scrollY"));
}

// Test select tool with Node ID targeting
IN_PROC_BROWSER_TEST_F(ContentAgentToolsTest, SelectTool_NodeIdTarget) {
  NavigateToChromiumTestFile("/actor/select_tool.html");

  auto select_tool = FindToolByName("select_dropdown");
  ASSERT_TRUE(select_tool);

  // Get initial selected value
  std::string initial_value =
      content::EvalJs(web_contents(),
                      "document.getElementById('plainSelect').value")
          .ExtractString();
  EXPECT_EQ("alpha", initial_value);

  // Get real DOM node ID for the select element
  int select_node_id = GetDOMNodeId("#plainSelect");
  auto target_dict = target_test_util::GetContentNodeTargetDict(
      select_node_id, GetMainFrameDocumentIdentifier());

  base::Value::Dict input;
  input.Set("target", target_dict.Clone());
  input.Set("value", "beta");

  auto result = ExecuteToolAndWait(select_tool, *base::WriteJson(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the correct value was selected
  std::string selected_value =
      content::EvalJs(web_contents(),
                      "document.getElementById('plainSelect').value")
          .ExtractString();
  EXPECT_EQ("beta", selected_value);
}

// Test navigation tool
IN_PROC_BROWSER_TEST_F(ContentAgentToolsTest, NavigationTool_BasicNavigation) {
  // Start with a basic page
  NavigateToChromiumTestFile("/actor/page_with_clickable_element.html");

  auto nav_tool = FindToolByName("web_page_navigator");
  ASSERT_TRUE(nav_tool);

  // Get initial URL
  GURL initial_url = web_contents()->GetVisibleURL();

  // Create input for navigating to a different test page
  GURL test_url = embedded_https_test_server().GetURL("/actor/input.html");
  base::Value::Dict input;
  input.Set("website_url", test_url.spec());

  auto result = ExecuteToolAndWait(nav_tool, *base::WriteJson(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the page navigated to the new URL
  GURL final_url = web_contents()->GetURL();
  EXPECT_NE(initial_url, final_url);
  EXPECT_EQ(test_url.path(), final_url.path());
}

IN_PROC_BROWSER_TEST_F(ContentAgentToolsTest, BlockExtensionStore) {
  // Verify that navigating to the extension store is blocked
  NavigateToChromiumTestFile("/actor/page_with_clickable_element.html");

  auto nav_tool = FindToolByName("web_page_navigator");
  ASSERT_TRUE(nav_tool);

  // Get initial URL
  GURL initial_url = web_contents()->GetVisibleURL();

  // Create input for navigating to a different test page
  base::Value::Dict input;
  input.Set("website_url", "https://chromewebstore.google.com/example");

  auto result = ExecuteToolAndWait(nav_tool, *base::WriteJson(input), false);
  EXPECT_GT(result.size(), 0u);
  EXPECT_THAT(result, ContentBlockText(testing::HasSubstr("Error")));

  // Verify the page could not navigate to the URL
  GURL final_url = web_contents()->GetURL();
  EXPECT_EQ(initial_url, final_url);

  // Also verify that other actions won't be able to execute against tabs
  // already on an extension store URL.
  base::test::TestFuture<actor::MayActOnUrlBlockReason> allowed;
  auto* actor_service =
      actor::ActorKeyedServiceFactory::GetActorKeyedService(agent_profile_);
  actor_service->GetPolicyChecker().MayActOnUrl(
      GURL("https://chromewebstore.google.com/example"), false, agent_profile_,
      actor_service->GetJournal(), actor::TaskId(), allowed.GetCallback());
  EXPECT_NE(allowed.Take(), actor::MayActOnUrlBlockReason::kAllowed);
}

// Test drag and release tool with coordinates (since drag needs from/to)
IN_PROC_BROWSER_TEST_F(ContentAgentToolsTest,
                       DragAndReleaseTool_CoordinateTargets) {
  NavigateToChromiumTestFile("/actor/drag.html");

  auto drag_tool = FindToolByName("drag_and_release");
  ASSERT_TRUE(drag_tool);

  // Get initial range value
  int initial_value =
      content::EvalJs(web_contents(),
                      "parseInt(document.getElementById('range').value)")
          .ExtractInt();
  EXPECT_EQ(0, initial_value);

  // Use coordinate targeting for drag operation (from start to middle of range)
  auto from_target =
      target_test_util::GetCoordinateTargetDict(25, 15);  // Start of range
  auto to_target =
      target_test_util::GetCoordinateTargetDict(100, 15);  // Middle of range

  base::Value::Dict input;
  input.Set("from", from_target.Clone());
  input.Set("to", to_target.Clone());

  auto result = ExecuteToolAndWait(drag_tool, *base::WriteJson(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the range value changed (exact value depends on drag implementation)
  int final_value =
      content::EvalJs(web_contents(),
                      "parseInt(document.getElementById('range').value)")
          .ExtractInt();
  EXPECT_NE(initial_value, final_value);
}

IN_PROC_BROWSER_TEST_F(ContentAgentToolsTest, HistoryTool_Back) {
  const GURL url_first =
      embedded_test_server()->GetURL("/actor/blank.html?start");
  const GURL url_second =
      embedded_test_server()->GetURL("/actor/blank.html?target");

  ASSERT_TRUE(content::NavigateToURL(web_contents(), url_first));
  ASSERT_TRUE(content::NavigateToURL(web_contents(), url_second));
  ASSERT_EQ(web_contents()->GetURL(), url_second);

  auto history_tool = FindToolByName("navigate_history");
  ASSERT_TRUE(history_tool);

  base::Value::Dict input;
  input.Set("direction", "back");
  auto result = ExecuteToolAndWait(history_tool, *base::WriteJson(input));
  EXPECT_GT(result.size(), 0u);
  EXPECT_EQ(web_contents()->GetURL(), url_first);
}

}  // namespace ai_chat
