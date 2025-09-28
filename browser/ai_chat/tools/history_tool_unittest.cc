// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/history_tool.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/tools/mock_content_agent_task_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "chrome/browser/actor/browser_action_util.h"
#include "chrome/browser/actor/task_id.h"
#include "chrome/browser/actor/tools/history_tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class HistoryToolTest : public testing::Test {
 public:
  void SetUp() override {
    mock_task_provider_ = std::make_unique<MockContentAgentTaskProvider>();
    history_tool_ =
        std::make_unique<HistoryTool>(mock_task_provider_.get(),
                                      nullptr);  // Actor service not used

    test_tab_handle_ = tabs::TabHandle(123);
    test_task_id_ = actor::TaskId(456);

    mock_task_provider_->SetTaskId(test_task_id_);

    ON_CALL(*mock_task_provider_, GetOrCreateTabHandleForTask)
        .WillByDefault(base::test::RunOnceCallback<0>(test_tab_handle_));
  }

 protected:
  // Creates a valid history JSON with the given direction
  std::string CreateValidHistoryJson(const std::string& direction) {
    base::Value::Dict dict;
    dict.Set("direction", direction);

    std::string json;
    base::JSONWriter::Write(dict, &json);
    return json;
  }

  void RunWithExpectedError(const std::string& input_json,
                            const std::string& expected_error) {
    // For error cases, the tool should not call the interesting task provider
    // methods Note: GetTaskId() may still be called as it's infrastructure, but
    // we don't care
    EXPECT_CALL(*mock_task_provider_, GetOrCreateTabHandleForTask).Times(0);
    EXPECT_CALL(*mock_task_provider_, ExecuteActions).Times(0);

    base::test::TestFuture<std::vector<mojom::ContentBlockPtr>> future;
    history_tool_->UseTool(input_json, future.GetCallback());

    auto result = future.Take();
    EXPECT_EQ(result.size(), 1u);
    ASSERT_TRUE(result[0]->is_text_content_block());
    EXPECT_EQ(result[0]->get_text_content_block()->text, expected_error);
  }

  // Verify history action properties and conversions
  void VerifyHistoryAction(const optimization_guide::proto::Actions& actions,
                           const std::string& expected_direction) {
    // Verify proto action
    EXPECT_EQ(actions.task_id(), test_task_id_.value());
    EXPECT_EQ(actions.actions_size(), 1);

    const auto& action = actions.actions(0);

    if (expected_direction == "back") {
      EXPECT_TRUE(action.has_back());
      const auto& back_action = action.back();
      EXPECT_EQ(back_action.tab_id(), test_tab_handle_.raw_value());

      // Verify CreateToolRequest works and produces correct HistoryToolRequest
      auto tool_request = actor::CreateToolRequest(action, nullptr);
      ASSERT_NE(tool_request, nullptr);

      auto* history_request =
          static_cast<actor::HistoryToolRequest*>(tool_request.get());
      EXPECT_EQ(history_request->GetTabHandle(), test_tab_handle_);
      EXPECT_EQ(history_request->direction_,
                actor::HistoryToolRequest::Direction::kBack);
    } else {
      EXPECT_TRUE(action.has_forward());
      const auto& forward_action = action.forward();
      EXPECT_EQ(forward_action.tab_id(), test_tab_handle_.raw_value());

      // Verify CreateToolRequest works and produces correct HistoryToolRequest
      auto tool_request = actor::CreateToolRequest(action, nullptr);
      ASSERT_NE(tool_request, nullptr);

      auto* history_request =
          static_cast<actor::HistoryToolRequest*>(tool_request.get());
      EXPECT_EQ(history_request->GetTabHandle(), test_tab_handle_);
      EXPECT_EQ(history_request->direction_,
                actor::HistoryToolRequest::Direction::kForward);
    }
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<MockContentAgentTaskProvider> mock_task_provider_;
  std::unique_ptr<HistoryTool> history_tool_;
  tabs::TabHandle test_tab_handle_;
  actor::TaskId test_task_id_;
};

TEST_F(HistoryToolTest, ValidInputBack) {
  std::string input_json = CreateValidHistoryJson("back");
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      }));

  history_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify history action properties
  VerifyHistoryAction(captured_actions, "back");
}

TEST_F(HistoryToolTest, ValidInputForward) {
  std::string input_json = CreateValidHistoryJson("forward");
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        std::move(callback).Run(CreateContentBlocksForText("Success"));
        run_loop.Quit();
      }));

  history_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify history action properties
  VerifyHistoryAction(captured_actions, "forward");
}

TEST_F(HistoryToolTest, InvalidJson) {
  RunWithExpectedError("{ invalid json }",
                       "Failed to parse input JSON. Please try again.");
}

TEST_F(HistoryToolTest, MissingDirection) {
  std::string input_json = R"({})";

  RunWithExpectedError(
      input_json, "Invalid or missing direction. Must be 'back' or 'forward'.");
}

TEST_F(HistoryToolTest, InvalidDirection) {
  std::string input_json = CreateValidHistoryJson("invalid_direction");

  RunWithExpectedError(
      input_json, "Invalid or missing direction. Must be 'back' or 'forward'.");
}

TEST_F(HistoryToolTest, InvalidDirectionType) {
  std::string input_json = R"({
    "direction": 123
  })";

  RunWithExpectedError(
      input_json, "Invalid or missing direction. Must be 'back' or 'forward'.");
}

TEST_F(HistoryToolTest, ToolMetadata) {
  EXPECT_EQ(history_tool_->Name(), "navigate_history");
  EXPECT_FALSE(std::string(history_tool_->Description()).empty());

  auto properties = history_tool_->InputProperties();
  EXPECT_TRUE(properties.has_value());

  auto required = history_tool_->RequiredProperties();
  EXPECT_TRUE(required.has_value());
  EXPECT_EQ(required->size(), 1u);  // direction
}

}  // namespace ai_chat
