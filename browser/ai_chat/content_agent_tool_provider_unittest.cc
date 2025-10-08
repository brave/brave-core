// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/content_agent_tool_provider.h"

#include <memory>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "chrome/browser/actor/actor_keyed_service.h"
#include "chrome/browser/actor/actor_task.h"
#include "chrome/browser/actor/ui/mocks/mock_actor_ui_state_manager.h"
#include "chrome/common/actor/action_result.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_contents_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

namespace {

using ::testing::_;

std::unique_ptr<actor::ui::ActorUiStateManagerInterface>
BuildUiStateManagerMock() {
  std::unique_ptr<actor::ui::MockActorUiStateManager> ui_state_manager =
      std::make_unique<actor::ui::MockActorUiStateManager>();
  ON_CALL(*ui_state_manager, OnUiEvent(_, _))
      .WillByDefault(
          [](actor::ui::AsyncUiEvent, actor::ui::UiCompleteCallback callback) {
            std::move(callback).Run(actor::MakeOkResult());
          });
  return ui_state_manager;
}

}  // namespace

class ContentAgentToolProviderTest : public testing::Test {
 public:
  ContentAgentToolProviderTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME),
        testing_profile_manager_(TestingBrowserProcess::GetGlobal()) {
    // Enable the AI Chat Agent Profile feature
    scoped_feature_list_.InitAndEnableFeature(
        ai_chat::features::kAIChatAgentProfile);
  }
  ~ContentAgentToolProviderTest() override = default;

  // testing::Test:
  void SetUp() override {
    ASSERT_TRUE(testing_profile_manager_.SetUp());
    profile_ = testing_profile_manager_.CreateTestingProfile("profile");

    actor_service_ = actor::ActorKeyedService::Get(profile_);
    actor_service_->SetActorUiStateManagerForTesting(BuildUiStateManagerMock());

    // Create ContentAgentToolProvider
    tool_provider_ = std::make_unique<ContentAgentToolProvider>(
        profile_, actor_service_.get());
  }

  // Helper to create an Actions proto for testing ExecuteActions
  optimization_guide::proto::Actions CreateTestActions(
      actor::TaskId task_id,
      tabs::TabHandle tab_handle) {
    optimization_guide::proto::Actions actions;
    actions.set_task_id(task_id.value());

    auto* action = actions.add_actions();
    auto* click = action->mutable_click();
    click->set_tab_id(tab_handle.raw_value());
    auto* target = click->mutable_target();
    target->set_content_node_id(123);

    return actions;
  }

  base::WeakPtr<Tool> FindToolByName(const std::string& name) {
    auto tools = tool_provider_->GetTools();
    for (auto& tool : tools) {
      if (tool && tool->Name() == name) {
        return tool;
      }
    }
    return nullptr;
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
  TestingProfileManager testing_profile_manager_;
  raw_ptr<TestingProfile> profile_;
  raw_ptr<actor::ActorKeyedService> actor_service_;
  std::unique_ptr<ContentAgentToolProvider> tool_provider_;
  content::TestWebContentsFactory web_contents_factory_;
};

// Test that ContentAgentToolProvider creates tools
TEST_F(ContentAgentToolProviderTest, CreateTools) {
  auto tools = tool_provider_->GetTools();
  EXPECT_GT(tools.size(), 0u);
  EXPECT_FALSE(tool_provider_->GetTaskId().is_null());

  // Verify some expected tools are present
  std::vector<std::string> expected_tools = {
      "click_element", "type_text", "scroll_element", "web_page_navigator"};

  for (const std::string& expected_name : expected_tools) {
    auto tool = FindToolByName(expected_name);
    EXPECT_TRUE(tool) << "Expected tool '" << expected_name << "' not found";
  }
}

// Test that StopAllTasks stops the task
TEST_F(ContentAgentToolProviderTest, StopAllTasks) {
  actor::TaskId task_id = tool_provider_->GetTaskId();
  EXPECT_FALSE(task_id.is_null());

  EXPECT_EQ(actor_service_->GetActiveTasks().count(task_id), 1u);
  EXPECT_EQ(actor_service_->GetInactiveTasks().count(task_id), 0u);

  tool_provider_->StopAllTasks();

  // Verify task is now in inactive tasks
  EXPECT_EQ(actor_service_->GetActiveTasks().count(task_id), 0u);
  EXPECT_EQ(actor_service_->GetInactiveTasks().count(task_id), 1u);
}

// NOTE: GetOrCreateTabHandleForTask or ExecuteActions with valid actions cannot
// be tested in unit tests because they require full WebContents and frame
// infrastructure. These are tested in browser tests, see:
// content_agent_tool_provider_browsertest.cc

// Test ExecuteActions with empty action sequence is handled from result
// of ActorKeyedService::PerformActions.
TEST_F(ContentAgentToolProviderTest, ExecuteActions_EmptyActionSequence) {
  base::test::TestFuture<std::vector<mojom::ContentBlockPtr>> result_future;

  optimization_guide::proto::Actions actions;
  actions.set_task_id(tool_provider_->GetTaskId().value());

  tool_provider_->ExecuteActions(actions, result_future.GetCallback());

  auto result = result_future.Take();

  ASSERT_GT(result.size(), 0u);
  EXPECT_TRUE(result[0]->is_text_content_block());
  EXPECT_THAT(result[0]->get_text_content_block()->text,
              testing::HasSubstr("Action failed - no actions specified"));
}

// Text ExecuteActions with an invalid action is handled before sending to
// ActorKeyedService::PerformActions.
TEST_F(ContentAgentToolProviderTest, ExecuteActions_InvalidAction) {
  base::test::TestFuture<std::vector<mojom::ContentBlockPtr>> result_future;

  // Create an Actions proto with an invalid action (no target)
  optimization_guide::proto::Actions actions;
  actions.set_task_id(tool_provider_->GetTaskId().value());
  auto* action = actions.add_actions();

  auto* click_action = action->mutable_click();
  click_action->set_tab_id(123);

  tool_provider_->ExecuteActions(actions, result_future.GetCallback());

  auto result = result_future.Take();

  ASSERT_GT(result.size(), 0u);
  EXPECT_TRUE(result[0]->is_text_content_block());
  EXPECT_THAT(result[0]->get_text_content_block()->text,
              testing::HasSubstr("Action failed - incorrect parameters"));
}

}  // namespace ai_chat
