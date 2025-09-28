// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <memory>
#include <string>
#include <vector>

#include "base/json/json_writer.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/browser/ai_chat/content_agent_tool_provider.h"
#include "brave/browser/ai_chat/tools/target_test_util.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/browser/actor/actor_keyed_service_factory.h"
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

// These tests verify, end to end, that the various content tools utilize
// the Chromium actor framework successfully. They do not need to test all
// edge cases with either the actor framework or the tool param parsing, since
// that is covered by the chromium actor browser tests and the brave tool unit
// tests.
class BrowserToolsTest : public InProcessBrowserTest {
 public:
  BrowserToolsTest() = default;
  ~BrowserToolsTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());
    ASSERT_TRUE(embedded_https_test_server().Start());

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

  // Helper to create JSON input for tool calls
  std::string CreateToolInput(const base::Value::Dict& input) {
    std::string json_string;
    base::JSONWriter::Write(input, &json_string);
    return json_string;
  }

  // Helper to execute a tool and wait for completion
  Tool::ToolResult ExecuteToolAndWait(base::WeakPtr<Tool> tool,
                                      const std::string& input_json) {
    base::test::TestFuture<Tool::ToolResult> result;
    tool->UseTool(input_json, result.GetCallback());
    return result.Take();
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
  Profile* GetProfile() { return browser()->profile(); }

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

  std::unique_ptr<ContentAgentToolProvider> tool_provider_;
};

// Test that BrowserToolProvider can be created and provides expected tools
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, ProviderCreation) {
  EXPECT_NE(tool_provider_, nullptr);

  auto tools = tool_provider_->GetTools();
  EXPECT_GT(tools.size(), 0u);

  // Verify some expected tools are present
  std::vector<std::string> expected_tools = {
      "click_element", "type_text", "scroll_element", "web_page_navigator"};

  for (const std::string& expected_name : expected_tools) {
    auto tool = FindToolByName(expected_name);
    EXPECT_TRUE(tool) << "Expected tool '" << expected_name << "' not found";
  }
}

// Test click tool with Node ID targeting
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, ClickTool_NodeIdTarget) {
  NavigateToChromiumTestFile("/actor/page_with_clickable_element.html");

  auto click_tool = FindToolByName("click_element");
  ASSERT_TRUE(click_tool);

  // Reset click tracking
  ASSERT_TRUE(content::ExecJs(web_contents(), "button_clicked = false"));
  ASSERT_TRUE(content::ExecJs(web_contents(), "mouse_event_log = []"));

  // Get real DOM node ID for the clickable button
  int button_node_id = GetDOMNodeId("button#clickable");

  auto target_dict = target_test_util::GetContentNodeTargetDict(
      button_node_id,
      *optimization_guide::DocumentIdentifierUserData::GetDocumentIdentifier(
          web_contents()->GetPrimaryMainFrame()->GetGlobalFrameToken()));

  base::Value::Dict input;
  input.Set("target", std::move(target_dict));
  input.Set("click_type", "left");
  input.Set("click_count", "single");

  auto result = ExecuteToolAndWait(click_tool, CreateToolInput(input));
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
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, TypeTool_NodeIdTarget) {
  NavigateToChromiumTestFile("/actor/input.html");

  auto type_tool = FindToolByName("type_text");
  ASSERT_TRUE(type_tool);

  // Reset input tracking and clear the input field
  ASSERT_TRUE(content::ExecJs(web_contents(), "input_event_log = []"));
  ASSERT_TRUE(content::ExecJs(web_contents(),
                              "document.getElementById('input').value = ''"));

  // Get real DOM node ID for the input element
  int input_node_id = GetDOMNodeId("#input");
  auto target_dict = target_test_util::GetContentNodeTargetDict(
      input_node_id,
      *optimization_guide::DocumentIdentifierUserData::GetDocumentIdentifier(
          web_contents()->GetPrimaryMainFrame()->GetGlobalFrameToken()));

  base::Value::Dict input;
  input.Set("target", std::move(target_dict));
  input.Set("text", "Hello World");
  input.Set("follow_by_enter", false);
  input.Set("mode", "replace");

  auto result = ExecuteToolAndWait(type_tool, CreateToolInput(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the text content was actually entered
  std::string input_value =
      content::EvalJs(web_contents(), "document.getElementById('input').value")
          .ExtractString();
  EXPECT_EQ("Hello World", input_value);
}

// Test scroll tool with Node ID targeting
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, ScrollTool_NodeIdTarget) {
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
      scroller_node_id,
      *optimization_guide::DocumentIdentifierUserData::GetDocumentIdentifier(
          web_contents()->GetPrimaryMainFrame()->GetGlobalFrameToken()));

  base::Value::Dict input;
  input.Set("target", std::move(target_dict));
  input.Set("direction", "down");
  input.Set("distance", 50);

  auto result = ExecuteToolAndWait(scroll_tool, CreateToolInput(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the element was scrolled down
  int final_scroll =
      content::EvalJs(web_contents(),
                      "document.getElementById('scroller').scrollTop")
          .ExtractInt();
  EXPECT_GT(final_scroll, initial_scroll);
}

// Test scroll tool with Node ID targeting
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, ScrollTool_DocumentTarget) {
  NavigateToChromiumTestFile("/actor/scrollable_page.html");

  auto scroll_tool = FindToolByName("scroll_element");
  ASSERT_TRUE(scroll_tool);

  int scroll_distance = 50;

  ASSERT_EQ(0, EvalJs(web_contents(), "window.scrollY"));

  auto target_dict = target_test_util::GetDocumentTargetDict(
      *optimization_guide::DocumentIdentifierUserData::GetDocumentIdentifier(
          web_contents()->GetPrimaryMainFrame()->GetGlobalFrameToken()));

  base::Value::Dict input;
  input.Set("target", std::move(target_dict));
  input.Set("direction", "down");
  input.Set("distance", scroll_distance);

  auto result = ExecuteToolAndWait(scroll_tool, CreateToolInput(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the element was scrolled down
  EXPECT_EQ(scroll_distance, EvalJs(web_contents(), "window.scrollY"));
}

// Test select tool with Node ID targeting
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, SelectTool_NodeIdTarget) {
  NavigateToChromiumTestFile("/actor/select_tool.html");

  auto select_tool = FindToolByName("select_dropdown");
  if (!select_tool) {
    GTEST_SKIP() << "select_dropdown tool not available";
  }

  // Get initial selected value
  std::string initial_value =
      content::EvalJs(web_contents(),
                      "document.getElementById('plainSelect').value")
          .ExtractString();
  EXPECT_EQ("alpha", initial_value);

  // Get real DOM node ID for the select element
  int select_node_id = GetDOMNodeId("#plainSelect");
  auto target_dict = target_test_util::GetContentNodeTargetDict(
      select_node_id,
      *optimization_guide::DocumentIdentifierUserData::GetDocumentIdentifier(
          web_contents()->GetPrimaryMainFrame()->GetGlobalFrameToken()));

  base::Value::Dict input;
  input.Set("target", std::move(target_dict));
  input.Set("value", "beta");

  auto result = ExecuteToolAndWait(select_tool, CreateToolInput(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the correct value was selected
  std::string selected_value =
      content::EvalJs(web_contents(),
                      "document.getElementById('plainSelect').value")
          .ExtractString();
  EXPECT_EQ("beta", selected_value);
}

// Test navigation tool
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, NavigationTool_BasicNavigation) {
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

  auto result = ExecuteToolAndWait(nav_tool, CreateToolInput(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the page navigated to the new URL
  GURL final_url = web_contents()->GetURL();
  EXPECT_NE(initial_url, final_url);
  EXPECT_EQ(test_url.path(), final_url.path());
}

// Test drag and release tool with coordinates (since drag needs from/to)
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, DragAndReleaseTool_CoordinateTargets) {
  NavigateToChromiumTestFile("/actor/drag.html");

  auto drag_tool = FindToolByName("drag_and_release");
  if (!drag_tool) {
    GTEST_SKIP() << "drag_and_release tool not available";
  }

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
  input.Set("from", std::move(from_target));
  input.Set("to", std::move(to_target));

  auto result = ExecuteToolAndWait(drag_tool, CreateToolInput(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the range value changed (exact value depends on drag implementation)
  int final_value =
      content::EvalJs(web_contents(),
                      "parseInt(document.getElementById('range').value)")
          .ExtractInt();
  EXPECT_NE(initial_value, final_value);
}

}  // namespace ai_chat
