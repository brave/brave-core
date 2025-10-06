// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/content_agent_tool_provider.h"

#include <memory>
#include <vector>

#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/ai_chat_agent_profile_helper.h"
#include "brave/browser/ai_chat/tools/target_test_util.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "brave/components/ai_chat/core/browser/utils.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "chrome/browser/actor/actor_keyed_service_factory.h"
#include "chrome/browser/actor/browser_action_util.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/optimization_guide/content/browser/page_content_proto_provider.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "net/dns/mock_host_resolver.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

class ContentAgentToolProviderBrowserTest : public InProcessBrowserTest {
 public:
  ContentAgentToolProviderBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(
        ai_chat::features::kAIChatAgentProfile);
  }

  ~ContentAgentToolProviderBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    host_resolver()->AddRule("*", "127.0.0.1");
    ASSERT_TRUE(embedded_test_server()->Start());

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

    // Create the tool provider
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
    // Ensure physical and css pixels are the same
    command_line->AppendSwitchASCII(switches::kForceDeviceScaleFactor, "1");
  }

 protected:
  Profile* GetProfile() { return agent_profile_; }

  // Helper to get web contents from tool provider
  content::WebContents* GetToolProviderWebContents() {
    base::test::TestFuture<tabs::TabHandle> tab_handle_future;
    tool_provider_->GetOrCreateTabHandleForTask(
        tab_handle_future.GetCallback());
    tabs::TabHandle tab_handle = tab_handle_future.Take();
    return tab_handle.Get()->GetContents();
  }

  // Helper to navigate tool provider's tab
  void NavigateToolProviderTab(const GURL& url) {
    content::WebContents* web_contents = GetToolProviderWebContents();
    ASSERT_TRUE(content::NavigateToURL(web_contents, url));
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
      std::optional<optimization_guide::AIPageContentResult> content) {
    tool_provider_->ReceivedAnnotatedPageContent(std::move(callback),
                                                 std::move(content));
  }

  void OnActionsFinished(
      Tool::UseToolCallback callback,
      actor::mojom::ActionResultCode result_code,
      std::optional<size_t> index_of_failed_action,
      std::vector<actor::ActionResultWithLatencyInfo> action_results) {
    tool_provider_->OnActionsFinished(std::move(callback), result_code,
                                      std::move(index_of_failed_action),
                                      std::move(action_results));
  }

  raw_ptr<Profile> agent_profile_;
  std::unique_ptr<ContentAgentToolProvider> tool_provider_;
  base::test::ScopedFeatureList scoped_feature_list_;
};

// End-to-end tests of ExecuteActions with valid actions are tested in
// `content_agent_tools_browsertest.cc`.

// Test that GetOrCreateTabHandleForTask returns valid and the same tab on
// subsequent calls.
IN_PROC_BROWSER_TEST_F(ContentAgentToolProviderBrowserTest,
                       GetOrCreateTabHandleForTask) {
  base::test::TestFuture<tabs::TabHandle> first_future;
  tool_provider_->GetOrCreateTabHandleForTask(first_future.GetCallback());
  tabs::TabHandle first_handle = first_future.Take();

  EXPECT_TRUE(first_handle.Get());
  EXPECT_TRUE(first_handle.Get()->GetContents());

  base::test::TestFuture<tabs::TabHandle> second_future;
  tool_provider_->GetOrCreateTabHandleForTask(second_future.GetCallback());
  tabs::TabHandle second_handle = second_future.Take();

  EXPECT_EQ(first_handle, second_handle);
  EXPECT_EQ(first_handle.Get()->GetContents(),
            second_handle.Get()->GetContents());
}

// Test calling ExecuteActions on a closed tab is handled
IN_PROC_BROWSER_TEST_F(ContentAgentToolProviderBrowserTest,
                       ExecuteActions_TabClosed) {
  GURL test_url = embedded_test_server()->GetURL("/actor/blank.html");
  NavigateToolProviderTab(test_url);

  // Create and execute the action
  base::test::TestFuture<tabs::TabHandle> tab_handle_future;
  tool_provider_->GetOrCreateTabHandleForTask(tab_handle_future.GetCallback());
  tabs::TabHandle tab_handle = tab_handle_future.Take();

  // Create and try to execute an action on the closed tab
  optimization_guide::proto::Actions actions =
      CreateClickAction(tab_handle, 0, 0);

  // Close the tab
  tab_handle.Get()->Close();

  base::test::TestFuture<std::vector<mojom::ContentBlockPtr>> result_future;
  tool_provider_->ExecuteActions(actions, result_future.GetCallback());

  auto result = result_future.Take();
  EXPECT_THAT(result, ContentBlockText(testing::HasSubstr(
                          "Action failed - incorrect parameters")));

  // We also want to verify that OnActionsFinished handles a closed tab. Perhaps
  // an Action closes a tab unexpectedly. We can't yet simulate an action which
  // does this, so let's call OnActionsFinished directly.
  OnActionsFinished(result_future.GetCallback(),
                    actor::mojom::ActionResultCode::kOk, std::nullopt, {});
  EXPECT_THAT(result_future.Take(),
              ContentBlockText(testing::HasSubstr("Tab is no longer open")));
}

// Test when receiving no annotated page content
IN_PROC_BROWSER_TEST_F(ContentAgentToolProviderBrowserTest,
                       ReceivedAnnotatedPageContent_NoAnnotatedPageContent) {
  // TODO(https://github.com/brave/brave-browser/issues/49928): Creating a tab
  // handle avoids race condition with browser window initializing.
  base::test::TestFuture<tabs::TabHandle> tab_handle_future;
  tool_provider_->GetOrCreateTabHandleForTask(tab_handle_future.GetCallback());
  ASSERT_TRUE(tab_handle_future.Wait());

  base::test::TestFuture<std::vector<mojom::ContentBlockPtr>> result_future;
  ReceivedAnnotatedPageContent(result_future.GetCallback(), std::nullopt);
  auto result = result_future.Take();
  EXPECT_THAT(result, ContentBlockText(
                          testing::HasSubstr("Error getting page content")));
}

// Test when receiving no root node
IN_PROC_BROWSER_TEST_F(ContentAgentToolProviderBrowserTest,
                       ReceivedAnnotatedPageContent_NoRootNode) {
  // TODO(https://github.com/brave/brave-browser/issues/49928): Creating a tab
  // handle avoids race condition with browser window initializing.
  base::test::TestFuture<tabs::TabHandle> tab_handle_future;
  tool_provider_->GetOrCreateTabHandleForTask(tab_handle_future.GetCallback());
  ASSERT_TRUE(tab_handle_future.Wait());

  base::test::TestFuture<std::vector<mojom::ContentBlockPtr>> result_future;
  optimization_guide::AIPageContentResult page_content;
  ReceivedAnnotatedPageContent(result_future.GetCallback(),
                               std::move(page_content));
  auto result = result_future.Take();

  EXPECT_THAT(result, ContentBlockText(testing::HasSubstr("No root node")));
}

}  // namespace ai_chat
