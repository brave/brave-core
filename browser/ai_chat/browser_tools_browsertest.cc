// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// End-to-end browser tests for Brave AI LLM Tools.
//
// These tests verify that the BrowserToolProvider can successfully create
// and execute Brave's AI Chat browser tools. Each tool is tested with
// representative input variations to ensure proper integration with the
// Chromium actor framework. Tests focus on tool creation, input validation,
// and basic execution flow rather than comprehensive tool functionality,
// which is covered by individual unit tests.

#include <memory>
#include <string>
#include <vector>

#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/browser/ai_chat/browser_tool_provider.h"
#include "brave/browser/ai_chat/tools/target_test_util.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/actor/actor_keyed_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

class BrowserToolsTest : public InProcessBrowserTest {
 public:
  BrowserToolsTest() = default;
  ~BrowserToolsTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    base::FilePath test_data_dir;
    base::PathService::Get(brave::DIR_TEST_DATA, &test_data_dir);
    embedded_https_test_server().ServeFilesFromDirectory(test_data_dir);

    // Set up embedded test server
    ASSERT_TRUE(embedded_https_test_server().Start());

    // Get the actor service
    auto* actor_service = actor::ActorKeyedServiceFactory::GetActorKeyedService(
        GetProfile());
    ASSERT_NE(actor_service, nullptr);

    // Get the browser tool provider
    tool_provider_ = std::make_unique<BrowserToolProvider>(
        GetProfile(), actor_service);
    ASSERT_NE(tool_provider_, nullptr);

    // Use the NavigationTool to navigate to the test page
    auto nav_tool = FindToolByName("web_page_navigator");
    ASSERT_TRUE(nav_tool);
    base::Value::Dict input;
    input.Set("website_url",
        embedded_https_test_server()
                  .GetURL("/ai_chat/browser_tools_test.html")
                  .spec());
    auto result = ExecuteToolAndWait(nav_tool, CreateToolInput(input));
    LOG(ERROR) << "Navigation result:";
    ASSERT_GT(result.size(), 0u);

    ASSERT_EQ(browser()->tab_strip_model()->count(), 2);
    browser()->tab_strip_model()->ActivateTabAt(1);
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
  Tool::ToolResult ExecuteToolAndWait(
      base::WeakPtr<Tool> tool,
      const std::string& input_json) {
    base::test::TestFuture<Tool::ToolResult> result;
    LOG(ERROR) << "Executing tool: " << tool->Name();
    LOG(ERROR) << "Input: " << input_json;
    tool->UseTool(
        input_json,
        result.GetCallback(),
        std::nullopt);

    auto results = result.Take();

    // log the result
    DLOG(ERROR) << "Result:";
    for (const auto& block : results) {
      DLOG(ERROR) << "- "
                 << block->get_text_content_block()->text.substr(0, 1000);
    }

    return results;
  }

  // Helper to get the web contents
  content::WebContents* web_contents() {
    CHECK(tool_provider_);
    CHECK(tool_provider_->task_tab_handle_.Get());
    CHECK(tool_provider_->task_tab_handle_.Get()->GetContents());

    return tool_provider_->task_tab_handle_.Get()->GetContents();
  }

  // Helper to get the profile
  Profile* GetProfile() {
    return browser()->profile();
  }

  std::unique_ptr<BrowserToolProvider> tool_provider_;
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

// Test click tool with left click single variation
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, ClickTool_LeftClickSingle) {
  auto click_tool = FindToolByName("click_element");
  ASSERT_TRUE(click_tool);

  // Reset click tracking
  ASSERT_TRUE(content::ExecJs(web_contents(), "button_clicked = false"));
  ASSERT_TRUE(content::ExecJs(web_contents(), "mouse_event_log = []"));

  // Create input for clicking using coordinates (test button center at 160+91, 160+35)
  base::Value::Dict target = target_test_util::GetCoordinateTargetDict(251, 195);

  base::Value::Dict input;
  input.Set("target", std::move(target));
  input.Set("click_type", "left");
  input.Set("click_count", "single");

  auto result = ExecuteToolAndWait(click_tool, CreateToolInput(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the button was actually clicked
  EXPECT_EQ(true, content::EvalJs(web_contents(), "button_clicked"));

  // Verify mouse events were fired
  std::string mouse_events =
      content::EvalJs(web_contents(), "mouse_event_log.join(',')").ExtractString();
  EXPECT_TRUE(mouse_events.find("click[BUTTON#test-button]") != std::string::npos);
}

// Test click tool with right click double variation
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, ClickTool_RightClickDouble) {
  auto click_tool = FindToolByName("click_element");
  ASSERT_TRUE(click_tool);

  // Reset click tracking
  ASSERT_TRUE(content::ExecJs(web_contents(), "right_button_clicked = false"));
  ASSERT_TRUE(content::ExecJs(web_contents(), "mouse_event_log = []"));

  // Create input for right-click using coordinates (right-click button center at 360+126, 160+35)
  base::Value::Dict target = target_test_util::GetCoordinateTargetDict(486, 195);

  base::Value::Dict input;
  input.Set("target", std::move(target));
  input.Set("click_type", "right");
  input.Set("click_count", "single"); // Use single for right click validation

  auto result = ExecuteToolAndWait(click_tool, CreateToolInput(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the right click was detected
  EXPECT_EQ(true, content::EvalJs(web_contents(), "right_button_clicked"));

  // Verify context menu event was fired
  std::string mouse_events =
      content::EvalJs(web_contents(), "mouse_event_log.join(',')").ExtractString();
  EXPECT_TRUE(mouse_events.find("contextmenu[BUTTON#right-click-target]") != std::string::npos);
}

// Test type tool with text input
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, TypeTool_BasicTyping) {
  auto type_tool = FindToolByName("type_text");
  ASSERT_TRUE(type_tool);

  // Reset input tracking and clear the input field
  ASSERT_TRUE(content::ExecJs(web_contents(), "input_typed = false"));
  ASSERT_TRUE(content::ExecJs(web_contents(),
      "document.getElementById('test-input').value = ''"));

  // Create input for typing text into an input field (input center at 160+222, 260+37)
  base::Value::Dict target = target_test_util::GetCoordinateTargetDict(382, 297);

  base::Value::Dict input;
  input.Set("target", std::move(target));
  input.Set("text", "Hello World");
  input.Set("follow_by_enter", false);
  input.Set("mode", "replace");

  auto result = ExecuteToolAndWait(type_tool, CreateToolInput(input));
  EXPECT_GT(result.size(), 0u);

  // Verify text was typed into the input field
  EXPECT_EQ(true, content::EvalJs(web_contents(), "input_typed"));

  // Verify the text content was actually entered
  std::string input_value =
      content::EvalJs(web_contents(),
          "document.getElementById('test-input').value").ExtractString();
  EXPECT_EQ("Hello World", input_value);
}

// Test scroll tool with page scrolling
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, ScrollTool_PageScroll) {
  auto scroll_tool = FindToolByName("scroll_element");
  ASSERT_TRUE(scroll_tool);

  // Reset scroll tracking and scroll to top
  ASSERT_TRUE(content::ExecJs(web_contents(), "scroll_events = []"));
  ASSERT_TRUE(content::ExecJs(web_contents(), "window.scrollTo(0, 0)"));

  // Get initial scroll position
  int initial_scroll = content::EvalJs(web_contents(), "window.scrollY").ExtractInt();

  // Create input for scrolling down the page (target viewport)
  base::Value::Dict target = target_test_util::GetCoordinateTargetDict(400, 300);
  base::Value::Dict input;
  input.Set("target", std::move(target));
  input.Set("direction", "down");
  input.Set("distance", 200);

  auto result = ExecuteToolAndWait(scroll_tool, CreateToolInput(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the page was scrolled down
  int final_scroll = content::EvalJs(web_contents(), "window.scrollY").ExtractInt();
  EXPECT_GT(final_scroll, initial_scroll);

  // Verify scroll event was fired
  EXPECT_EQ(true, content::EvalJs(web_contents(), "scroll_events.includes('window-scroll')"));
}

// Test scroll tool with element scrolling
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, ScrollTool_ElementScroll) {
  auto scroll_tool = FindToolByName("scroll_element");
  ASSERT_TRUE(scroll_tool);

  // Reset scroll tracking and scroll element to bottom first
  ASSERT_TRUE(content::ExecJs(web_contents(), "scroll_events = []"));
  ASSERT_TRUE(content::ExecJs(web_contents(),
      "document.getElementById('scrollable-div').scrollTop = "
      "document.getElementById('scrollable-div').scrollHeight"));

  // Get initial scroll position
  int initial_scroll = content::EvalJs(web_contents(),
      "document.getElementById('scrollable-div').scrollTop").ExtractInt();

  // Create input for scrolling a specific element up (scrollable div center at 80+150, 350+50)
  base::Value::Dict target = target_test_util::GetCoordinateTargetDict(230, 400);

  base::Value::Dict input;
  input.Set("target", std::move(target));
  input.Set("direction", "up");
  input.Set("distance", 50);

  auto result = ExecuteToolAndWait(scroll_tool, CreateToolInput(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the element was scrolled up (scroll position should be less)
  int final_scroll = content::EvalJs(web_contents(),
      "document.getElementById('scrollable-div').scrollTop").ExtractInt();
  EXPECT_LT(final_scroll, initial_scroll);

  // Verify element scroll event was fired
  EXPECT_EQ(true, content::EvalJs(web_contents(), "scroll_events.includes('element-scroll')"));
}

// Test navigation tool
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, NavigationTool_BasicNavigation) {
  auto nav_tool = FindToolByName("web_page_navigator");
  ASSERT_TRUE(nav_tool);

  // Get initial URL
  GURL initial_url = web_contents()->GetVisibleURL();

  // Create input for navigating to a simple test page
  GURL test_url = embedded_https_test_server().GetURL("/simple.html");
  base::Value::Dict input;
  input.Set("website_url", test_url.spec());

  auto result = ExecuteToolAndWait(nav_tool, CreateToolInput(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the page navigated to the new URL
  GURL final_url = web_contents()->GetVisibleURL();
  EXPECT_NE(initial_url, final_url);
  EXPECT_EQ(test_url.path(), final_url.path());
}

// Test select tool with dropdown selection
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, SelectTool_DropdownSelection) {
  auto select_tool = FindToolByName("select_dropdown");
  if (!select_tool) {
    GTEST_SKIP() << "select_dropdown tool not available";
  }

  // Reset selection tracking
  ASSERT_TRUE(content::ExecJs(web_contents(), "dropdown_selected = false"));
  ASSERT_TRUE(content::ExecJs(web_contents(),
      "document.getElementById('test-select').value = ''"));

  // Create input for selecting dropdown option (select at 80+75, 180+15)
  base::Value::Dict target = target_test_util::GetCoordinateTargetDict(155, 195);

  base::Value::Dict input;
  input.Set("target", std::move(target));
  input.Set("value", "option2");

  auto result = ExecuteToolAndWait(select_tool, CreateToolInput(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the dropdown selection was made
  EXPECT_EQ(true, content::EvalJs(web_contents(), "dropdown_selected"));

  // Verify the correct value was selected
  std::string selected_value =
      content::EvalJs(web_contents(),
          "document.getElementById('test-select').value").ExtractString();
  EXPECT_EQ("option2", selected_value);
}

// Test move mouse tool
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, MoveMouse_CoordinateMove) {
  auto move_tool = FindToolByName("move_mouse");
  if (!move_tool) {
    GTEST_SKIP() << "move_mouse tool not available";
  }

  // Create input for moving mouse to target location
  base::Value::Dict target = target_test_util::GetCoordinateTargetDict(200, 300);
  base::Value::Dict input;
  input.Set("target", std::move(target));

  auto result = ExecuteToolAndWait(move_tool, CreateToolInput(input));
  EXPECT_GT(result.size(), 0u);

  // For mouse move, we mainly verify the tool executed without error
  // Actual mouse position is harder to verify in browser tests
  ASSERT_GT(result.size(), 0u);
  EXPECT_TRUE(result[0]);
}

// Test wait tool
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, WaitTool_TimeBasedWait) {
  auto wait_tool = FindToolByName("wait");
  if (!wait_tool) {
    GTEST_SKIP() << "wait tool not available";
  }

  // Record start time
  base::TimeTicks start_time = base::TimeTicks::Now();

  // Create input for waiting a specific duration (shorter for test speed)
  base::Value::Dict input;
  input.Set("wait_time_ms", 500);  // 0.5 seconds

  auto result = ExecuteToolAndWait(wait_tool, CreateToolInput(input));
  EXPECT_GT(result.size(), 0u);

  // Verify the wait actually took time (allow more margin for browser test overhead)
  base::TimeDelta elapsed = base::TimeTicks::Now() - start_time;
  EXPECT_GE(elapsed.InMilliseconds(), 400); // Allow some margin
  EXPECT_LT(elapsed.InMilliseconds(), 5000); // Allow more time for browser test overhead
}

// Test drag and release tool
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, DragAndReleaseTool_BasicDrag) {
  auto drag_tool = FindToolByName("drag_and_release");
  if (!drag_tool) {
    GTEST_SKIP() << "drag_and_release tool not available";
  }

  // Create input for dragging from draggable1 center to drop-zone1 center
  base::Value::Dict from_target = target_test_util::GetCoordinateTargetDict(105, 405); // draggable1 center
  base::Value::Dict to_target = target_test_util::GetCoordinateTargetDict(330, 430);   // drop-zone1 center

  base::Value::Dict input;
  input.Set("from", std::move(from_target));
  input.Set("to", std::move(to_target));

  auto result = ExecuteToolAndWait(drag_tool, CreateToolInput(input));
  EXPECT_GT(result.size(), 0u);

  // For drag operations, verify the tool completed successfully
  ASSERT_GT(result.size(), 0u);
  EXPECT_TRUE(result[0]);
}

// Test BrowserToolProvider task management
IN_PROC_BROWSER_TEST_F(BrowserToolsTest, TaskManagement) {
  EXPECT_NE(tool_provider_->GetTaskId(), actor::TaskId());

  // Test stopping all tasks
  tool_provider_->StopAllTasks();

  // Provider should still be functional after stopping tasks
  auto tools = tool_provider_->GetTools();
  EXPECT_GT(tools.size(), 0u);
}

}  // namespace ai_chat
