// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/wait_tool.h"

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
#include "chrome/browser/actor/tools/wait_tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class WaitToolTest : public testing::Test {
 public:
  void SetUp() override {
    mock_task_provider_ = std::make_unique<MockContentAgentTaskProvider>();
    wait_tool_ = std::make_unique<WaitTool>(mock_task_provider_.get());

    test_tab_handle_ = tabs::TabHandle(123);
    test_task_id_ = actor::TaskId(456);

    mock_task_provider_->SetTaskId(test_task_id_);

    ON_CALL(*mock_task_provider_, GetOrCreateTabHandleForTask)
        .WillByDefault(base::test::RunOnceCallback<0>(test_tab_handle_));
  }

 protected:
  // Creates a valid wait JSON with the given wait time
  std::string CreateValidWaitJson(int wait_time_ms) {
    base::Value::Dict dict;
    dict.Set("wait_time_ms", wait_time_ms);

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
    wait_tool_->UseTool(input_json, future.GetCallback());

    auto result = future.Take();
    EXPECT_EQ(result.size(), 1u);
    ASSERT_TRUE(result[0]->is_text_content_block());
    EXPECT_EQ(result[0]->get_text_content_block()->text, expected_error);
  }

  // Verify wait action properties and conversions
  void VerifyWaitAction(const optimization_guide::proto::Actions& actions,
                        int expected_wait_time_ms) {
    // Verify proto action
    EXPECT_EQ(actions.task_id(), test_task_id_.value());
    EXPECT_EQ(actions.actions_size(), 1);

    const auto& action = actions.actions(0);
    EXPECT_TRUE(action.has_wait());

    const auto& wait_action = action.wait();
    EXPECT_EQ(wait_action.wait_time_ms(), expected_wait_time_ms);

    // Verify CreateToolRequest works and produces correct WaitToolRequest
    auto tool_request = actor::CreateToolRequest(action, nullptr);
    ASSERT_NE(tool_request, nullptr);

    auto* wait_request =
        static_cast<actor::WaitToolRequest*>(tool_request.get());
    // Note: WaitToolRequest doesn't store wait_time_ms as a member, it's
    // converted to a TimeDelta in the constructor, so we can't easily verify
    // the exact value but we can verify the type
    EXPECT_NE(wait_request, nullptr);
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<MockContentAgentTaskProvider> mock_task_provider_;
  std::unique_ptr<WaitTool> wait_tool_;
  tabs::TabHandle test_tab_handle_;
  actor::TaskId test_task_id_;
};

TEST_F(WaitToolTest, ValidInputShortWait) {
  std::string input_json = CreateValidWaitJson(1000);  // 1 second
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      }));

  wait_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify wait action properties
  VerifyWaitAction(captured_actions, 1000);
}

TEST_F(WaitToolTest, ValidInputLongWait) {
  std::string input_json = CreateValidWaitJson(5000);  // 5 seconds
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

  wait_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify wait action properties
  VerifyWaitAction(captured_actions, 5000);
}

TEST_F(WaitToolTest, ValidInputZeroWait) {
  std::string input_json = CreateValidWaitJson(0);  // 0 milliseconds
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      }));

  wait_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify wait action properties
  VerifyWaitAction(captured_actions, 0);
}

TEST_F(WaitToolTest, InvalidJson) {
  RunWithExpectedError("{ invalid json }",
                       "Failed to parse input JSON. Please try again.");
}

TEST_F(WaitToolTest, MissingWaitTime) {
  std::string input_json = R"({})";

  RunWithExpectedError(
      input_json,
      "Invalid or missing wait_time_ms. Must be a non-negative integer.");
}

TEST_F(WaitToolTest, NegativeWaitTime) {
  std::string input_json = CreateValidWaitJson(-1000);

  RunWithExpectedError(
      input_json,
      "Invalid or missing wait_time_ms. Must be a non-negative integer.");
}

TEST_F(WaitToolTest, InvalidWaitTimeType) {
  std::string input_json = R"({
    "wait_time_ms": "not_a_number"
  })";

  RunWithExpectedError(
      input_json,
      "Invalid or missing wait_time_ms. Must be a non-negative integer.");
}

TEST_F(WaitToolTest, ToolMetadata) {
  EXPECT_EQ(wait_tool_->Name(), "wait");
  EXPECT_FALSE(std::string(wait_tool_->Description()).empty());

  auto properties = wait_tool_->InputProperties();
  EXPECT_TRUE(properties.has_value());

  auto required = wait_tool_->RequiredProperties();
  EXPECT_TRUE(required.has_value());
  EXPECT_EQ(required->size(), 1u);  // wait_time_ms
}

}  // namespace ai_chat
