// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/content_agent_tool_provider.h"

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/browser/ai_chat/ai_chat_agent_profile_helper.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "brave/components/constants/brave_paths.h"
#include "chrome/browser/actor/actor_keyed_service_factory.h"
#include "chrome/browser/actor/actor_proto_conversion.h"
#include "chrome/browser/actor/tab_observation_strategy.h"
#include "chrome/browser/glic/actor/glic_actor_policy_checker.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/common/chrome_features.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/actor/core/actor_features.h"
#include "components/optimization_guide/content/browser/page_content_proto_provider.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace ai_chat {

namespace {

// Concatenate the text of all text content blocks in a tool result.
std::string GetContentBlocksText(const Tool::ToolResult& blocks) {
  std::string text;
  for (const auto& block : blocks) {
    if (block && block->is_text_content_block()) {
      base::StrAppend(&text, {block->get_text_content_block()->text, "\n"});
    }
  }
  return text;
}

// Returns the single line within `text` that contains `needle`, or an empty
// string if not found. The page structure serializes each element's opening
// tag (with all of its attributes) on its own line, so this lets us assert on
// the attributes of a specific element.
std::string GetLineContaining(const std::string& text,
                              const std::string& needle) {
  size_t pos = text.find(needle);
  if (pos == std::string::npos) {
    return std::string();
  }
  size_t line_start = text.rfind('\n', pos);
  line_start = (line_start == std::string::npos) ? 0 : line_start + 1;
  size_t line_end = text.find('\n', pos);
  if (line_end == std::string::npos) {
    line_end = text.size();
  }
  return text.substr(line_start, line_end - line_start);
}

// Returns the opening tag line of the element whose visible text is `text`.
// Visible text is serialized as a nested <text> node, so the element that owns
// the text (e.g. a button) is the nearest preceding open tag that is neither a
// closing tag nor a <text> wrapper. This lets a test target an element the way
// a user would refer to it - by its visible label - rather than by an
// incidental attribute.
std::string GetElementLineForText(const std::string& structure,
                                  std::string_view text) {
  const std::vector<std::string_view> lines = base::SplitStringPiece(
      structure, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  size_t text_line = lines.size();
  for (size_t i = 0; i < lines.size(); ++i) {
    if (lines[i].find(text) != std::string_view::npos) {
      text_line = i;
      break;
    }
  }
  if (text_line == lines.size()) {
    return std::string();
  }
  for (size_t i = text_line + 1; i-- > 0;) {
    std::string_view line =
        base::TrimWhitespaceASCII(lines[i], base::TRIM_LEADING);
    if (base::StartsWith(line, "<") && !base::StartsWith(line, "</") &&
        !base::StartsWith(line, "<text")) {
      return std::string(line);
    }
  }
  return std::string();
}

}  // namespace

class ContentAgentToolProviderBrowserTest : public InProcessBrowserTest {
 public:
  ContentAgentToolProviderBrowserTest() {
    scoped_feature_list_.InitWithFeatures(
        /*enabled_features=*/{features::kAIChatAgentProfile},
        /*disabled_features=*/{actor::kGlicCrossOriginNavigationGating,
                               ::features::kGlicConfirmTabClose});
  }

  ~ContentAgentToolProviderBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    // Also serve Brave's test data (in addition to the default chrome/test/data
    // handlers) so tests can load fixtures from //brave/test/data/leo. The
    // navigation tool only navigates to https:// URLs, so serve over https too.
    const base::FilePath leo_dir =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA).AppendASCII("leo");
    embedded_test_server()->ServeFilesFromDirectory(leo_dir);
    embedded_https_test_server().ServeFilesFromDirectory(leo_dir);
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
    agent_browser_window_ = browser;

    // Get the actor service
    auto* actor_service =
        actor::ActorKeyedServiceFactory::GetActorKeyedService(GetProfile());
    ASSERT_NE(actor_service, nullptr);

    // Create the tool provider
    tool_provider_ =
        std::make_unique<ContentAgentToolProvider>(GetProfile(), actor_service);
    ASSERT_NE(tool_provider_, nullptr);
  }

  void TearDownOnMainThread() override {
    tool_provider_.reset();
    agent_profile_ = nullptr;
    agent_browser_window_ = nullptr;
    InProcessBrowserTest::TearDownOnMainThread();
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    // Ensure physical and css pixels are the same
    command_line->AppendSwitchASCII(switches::kForceDeviceScaleFactor, "1");
  }

 protected:
  Profile* GetProfile() { return agent_profile_; }

  // Helper to get web contents from tool provider
  tabs::TabHandle GetToolProviderTabHandle() {
    base::test::TestFuture<tabs::TabHandle> tab_handle_future;
    tool_provider_->GetOrCreateTabHandleForTask(
        tab_handle_future.GetCallback());
    return tab_handle_future.Take();
  }

  // Helper to navigate tool provider's tab
  void NavigateToolProviderTab(const GURL& url) {
    tabs::TabHandle tab_handle = GetToolProviderTabHandle();
    ASSERT_TRUE(tab_handle.Get());
    content::WebContents* web_contents = tab_handle.Get()->GetContents();
    ASSERT_TRUE(content::NavigateToURL(web_contents, url));
  }

  // Helper to find a tool by name
  base::WeakPtr<Tool> FindToolByName(std::string_view name) {
    for (auto& tool : tool_provider_->GetTools()) {
      if (tool && tool->Name() == name) {
        return tool;
      }
    }
    return nullptr;
  }

  // Navigates the agent's tab to `url` using the navigation tool and returns
  // the serialized page structure (the XML representation) from the tool
  // result. The surrounding page metadata and interaction instructions are
  // excluded so that assertions only match the page structure and not, for
  // example, the page title or the description of the
  // "dom_id"/"clickable"/"editable" attributes.
  std::string NavigateAndGetPageStructure(const GURL& url) {
    // TODO(https://github.com/brave/brave-browser/issues/49928): Creating a tab
    // handle first avoids a race condition with the browser window
    // initializing.
    EXPECT_TRUE(GetToolProviderTabHandle().Get());

    base::WeakPtr<Tool> navigation_tool =
        FindToolByName(mojom::kNavigateToolName);
    EXPECT_TRUE(navigation_tool);
    if (!navigation_tool) {
      return std::string();
    }

    base::DictValue input;
    input.Set("website_url", url.spec());
    base::test::TestFuture<Tool::ToolResult, Tool::ToolArtifacts> result_future;
    navigation_tool->UseTool(*base::WriteJson(input),
                             result_future.GetCallback());
    auto [result, artifacts] = result_future.Take();
    EXPECT_TRUE(artifacts.empty());

    const std::string page_content = GetContentBlocksText(result);
    static constexpr char kStructureHeader[] =
        "=== PAGE STRUCTURE (XML representation) ===";
    const size_t start = page_content.find(kStructureHeader);
    if (start == std::string::npos) {
      return std::string();
    }
    const size_t end = page_content.find("=== INTERACTION INSTRUCTIONS", start);
    return page_content.substr(start, end - start);
  }

  // Helper to create a valid Actions proto for testing
  optimization_guide::proto::Actions
  CreateClickAction(tabs::TabHandle tab_handle, int x, int y) {
    optimization_guide::proto::Actions actions;
    actions.set_task_id(tool_provider_->GetTaskId().value());

    auto* action = actions.add_actions();
    auto* click = action->mutable_click();
    click->set_tab_id(tab_handle.raw_value());

    auto* target = click->mutable_target();
    target->mutable_coordinate()->set_x(x);
    target->mutable_coordinate()->set_y(y);

    return actions;
  }

  void ReceivedAnnotatedPageContent(
      Tool::UseToolCallback callback,
      optimization_guide::AIPageContentResultOrError content) {
    tool_provider_->ReceivedAnnotatedPageContent(std::move(callback),
                                                 std::move(content));
  }

  void OnActionsFinished(
      Tool::UseToolCallback callback,
      actor::mojom::ActionResultCode result_code,
      std::optional<size_t> index_of_failed_action,
      std::vector<actor::ActionResultWithLatencyInfo> action_results) {
    tool_provider_->OnActionsFinished(std::move(callback),
                                      std::move(action_results),
                                      actor::TabObservationStrategy());
  }

  raw_ptr<Profile> agent_profile_;
  raw_ptr<BrowserWindowInterface> agent_browser_window_;
  std::unique_ptr<ContentAgentToolProvider> tool_provider_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

// End-to-end tests of ExecuteActions with valid actions are tested in
// `content_agent_tools_browsertest.cc`.

// Test that GetOrCreateTabHandleForTask returns valid and the same tab on
// subsequent calls.
IN_PROC_BROWSER_TEST_F(ContentAgentToolProviderBrowserTest,
                       GetOrCreateTabHandleForTask) {
  auto initial_tab_count = agent_browser_window_->GetTabStripModel()->count();

  tabs::TabHandle first_handle = GetToolProviderTabHandle();
  ASSERT_TRUE(first_handle.Get());

  // First call should result in a new tab
  EXPECT_EQ(agent_browser_window_->GetTabStripModel()->count(),
            initial_tab_count + 1);

  // Should be on the blank page
  content::WaitForLoadStop(first_handle.Get()->GetContents());
  EXPECT_EQ(first_handle.Get()->GetContents()->GetLastCommittedURL(),
            url::kAboutBlankURL);

  tabs::TabHandle second_handle = GetToolProviderTabHandle();
  ASSERT_TRUE(second_handle.Get());

  EXPECT_EQ(first_handle, second_handle);

  // Should not have opened a second new tab
  EXPECT_EQ(agent_browser_window_->GetTabStripModel()->count(),
            initial_tab_count + 1);
}

// Test calling ExecuteActions on a closed tab is handled
IN_PROC_BROWSER_TEST_F(ContentAgentToolProviderBrowserTest,
                       ExecuteActions_TabClosed) {
  GURL test_url = embedded_test_server()->GetURL("/actor/blank.html");
  NavigateToolProviderTab(test_url);

  tabs::TabHandle tab_handle = GetToolProviderTabHandle();
  ASSERT_TRUE(tab_handle.Get());

  optimization_guide::proto::Actions actions =
      CreateClickAction(tab_handle, 0, 0);

  // Close the tab
  tab_handle.Get()->Close();
  ASSERT_TRUE(base::test::RunUntil([&]() { return !tab_handle.Get(); }));

  base::test::TestFuture<Tool::ToolResult, Tool::ToolArtifacts> result_future;
  tool_provider_->ExecuteActions(actions, result_future.GetCallback());

  auto [result, artifacts] = result_future.Take();
  EXPECT_TRUE(artifacts.empty());
  EXPECT_THAT(result, ContentBlockText(testing::HasSubstr(
                          "Error: action failed - incorrect parameters")));

  // We also want to verify that OnActionsFinished handles a closed tab. Perhaps
  // an Action closes a tab unexpectedly. We can't yet simulate an action which
  // does this, so let's call OnActionsFinished directly.
  OnActionsFinished(result_future.GetCallback(),
                    actor::mojom::ActionResultCode::kOk, std::nullopt, {});
  auto [result_2, artifacts_2] = result_future.Take();
  EXPECT_TRUE(artifacts_2.empty());
  EXPECT_THAT(result_2,
              ContentBlockText(testing::HasSubstr("tab is no longer open")));
}

// Test when receiving no annotated page content
IN_PROC_BROWSER_TEST_F(ContentAgentToolProviderBrowserTest,
                       ReceivedAnnotatedPageContent_NoAnnotatedPageContent) {
  // TODO(https://github.com/brave/brave-browser/issues/49928): Creating a tab
  // handle avoids race condition with browser window initializing.
  base::test::TestFuture<tabs::TabHandle> tab_handle_future;
  tool_provider_->GetOrCreateTabHandleForTask(tab_handle_future.GetCallback());
  ASSERT_TRUE(tab_handle_future.Wait());

  base::test::TestFuture<Tool::ToolResult, Tool::ToolArtifacts> result_future;
  ReceivedAnnotatedPageContent(result_future.GetCallback(),
                               base::unexpected("Uninitialized"));
  auto [result, artifacts] = result_future.Take();
  EXPECT_TRUE(artifacts.empty());
  EXPECT_THAT(result, ContentBlockText(
                          testing::HasSubstr("could not get page content")));
}

// Test when receiving no root node
IN_PROC_BROWSER_TEST_F(ContentAgentToolProviderBrowserTest,
                       ReceivedAnnotatedPageContent_NoRootNode) {
  // TODO(https://github.com/brave/brave-browser/issues/49928): Creating a tab
  // handle avoids race condition with browser window initializing.
  base::test::TestFuture<tabs::TabHandle> tab_handle_future;
  tool_provider_->GetOrCreateTabHandleForTask(tab_handle_future.GetCallback());
  ASSERT_TRUE(tab_handle_future.Wait());

  base::test::TestFuture<Tool::ToolResult, Tool::ToolArtifacts> result_future;
  optimization_guide::AIPageContentResult page_content;
  ReceivedAnnotatedPageContent(result_future.GetCallback(),
                               base::ok(std::move(page_content)));
  auto [result, artifacts] = result_future.Take();
  EXPECT_TRUE(artifacts.empty());

  EXPECT_THAT(result, ContentBlockText(testing::HasSubstr("No root node")));
}

// The navigation tool's page content reports text input form controls (the
// title and description fields) with a dom_id and as clickable, and exposes
// their editable region as a separate node with a dom_id and the editable
// attribute. The fields are nested inside several single-child containers.
IN_PROC_BROWSER_TEST_F(ContentAgentToolProviderBrowserTest,
                       PageContentReportsTargetableFormControls) {
  const std::string structure = NavigateAndGetPageStructure(
      embedded_https_test_server().GetURL("/content_agent_form.html"));
  SCOPED_TRACE(structure);

  // The title text input is targetable (has a dom_id) and clickable.
  const std::string title_line =
      GetLineContaining(structure, "placeholder=\"Title\"");
  ASSERT_FALSE(title_line.empty());
  EXPECT_THAT(title_line, testing::HasSubstr("<input"));
  EXPECT_THAT(title_line, testing::HasSubstr("dom_id="));
  EXPECT_THAT(title_line, testing::HasSubstr("clickable"));

  // The description textarea is likewise targetable and clickable.
  const std::string description_line =
      GetLineContaining(structure, "placeholder=\"Describe the issue\"");
  ASSERT_FALSE(description_line.empty());
  EXPECT_THAT(description_line, testing::HasSubstr("<input"));
  EXPECT_THAT(description_line, testing::HasSubstr("dom_id="));
  EXPECT_THAT(description_line, testing::HasSubstr("clickable"));

  // The editable region of a form control is targetable for text input: it has
  // a dom_id and the editable attribute.
  const std::string editable_line = GetLineContaining(structure, " editable");
  ASSERT_FALSE(editable_line.empty());
  EXPECT_THAT(editable_line, testing::HasSubstr("dom_id="));
}

// The navigation tool's page content reports the submit button with a dom_id
// and as clickable, so it can be targeted.
IN_PROC_BROWSER_TEST_F(ContentAgentToolProviderBrowserTest,
                       PageContentReportsTargetableButton) {
  const std::string structure = NavigateAndGetPageStructure(
      embedded_https_test_server().GetURL("/content_agent_form.html"));
  SCOPED_TRACE(structure);

  // The submit button (identified by its visible "Create" text) is targetable
  // (has a dom_id) and clickable.
  const std::string button_line = GetElementLineForText(structure, "Create");
  ASSERT_FALSE(button_line.empty());
  EXPECT_THAT(button_line, testing::HasSubstr("<input"));
  EXPECT_THAT(button_line, testing::HasSubstr("dom_id="));
  EXPECT_THAT(button_line, testing::HasSubstr("clickable"));
}

}  // namespace ai_chat
