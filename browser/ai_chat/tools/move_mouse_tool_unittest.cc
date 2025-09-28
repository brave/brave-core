// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/move_mouse_tool.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/tools/mock_content_agent_task_provider.h"
#include "brave/browser/ai_chat/tools/target_test_util.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "chrome/browser/actor/browser_action_util.h"
#include "chrome/browser/actor/task_id.h"
#include "chrome/browser/actor/tools/move_mouse_tool_request.h"
#include "chrome/browser/actor/tools/page_tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class MoveMouseToolTest : public testing::Test {
 public:
  void SetUp() override {
    mock_task_provider_ = std::make_unique<MockContentAgentTaskProvider>();
    move_mouse_tool_ =
        std::make_unique<MoveMouseTool>(mock_task_provider_.get(),
                                        nullptr);  // Actor service not used

    test_tab_handle_ = tabs::TabHandle(123);
    test_task_id_ = actor::TaskId(456);

    mock_task_provider_->SetTaskId(test_task_id_);

    ON_CALL(*mock_task_provider_, GetOrCreateTabHandleForTask)
        .WillByDefault(base::test::RunOnceCallback<0>(test_tab_handle_));
  }

 protected:
  // Creates a valid move mouse JSON with the given target
  std::string CreateValidMoveMouseJson(const base::Value::Dict& target_dict) {
    base::Value::Dict dict;
    dict.Set("target", target_dict.Clone());

    std::string json;
    base::JSONWriter::Write(dict, &json);
    return json;
  }

  std::string CreateInvalidTargetJson(const std::string& target_content) {
    return R"({
      "target": )" +
           target_content + R"(
    })";
  }

  void RunWithExpectedError(const std::string& input_json,
                            const std::string& expected_error) {
    // For error cases, the tool should not call the interesting task provider
    // methods Note: GetTaskId() may still be called as it's infrastructure, but
    // we don't care
    EXPECT_CALL(*mock_task_provider_, GetOrCreateTabHandleForTask).Times(0);
    EXPECT_CALL(*mock_task_provider_, ExecuteActions).Times(0);

    base::test::TestFuture<std::vector<mojom::ContentBlockPtr>> future;
    move_mouse_tool_->UseTool(input_json, future.GetCallback());

    auto result = future.Take();
    EXPECT_EQ(result.size(), 1u);
    ASSERT_TRUE(result[0]->is_text_content_block());
    EXPECT_EQ(result[0]->get_text_content_block()->text, expected_error);
  }

  // Verify move mouse action properties and conversions
  void VerifyMoveMouseAction(
      const optimization_guide::proto::Actions& actions) {
    // Verify proto action
    EXPECT_EQ(actions.task_id(), test_task_id_.value());
    EXPECT_EQ(actions.actions_size(), 1);

    const auto& action = actions.actions(0);
    EXPECT_TRUE(action.has_move_mouse());

    const auto& move_mouse_action = action.move_mouse();
    EXPECT_EQ(move_mouse_action.tab_id(), test_tab_handle_.raw_value());

    // Target verification should be handled by the target_test_util methods
    EXPECT_TRUE(move_mouse_action.has_target());

    // Verify CreateToolRequest works and produces correct MoveMouseToolRequest
    auto tool_request = actor::CreateToolRequest(action, nullptr);
    ASSERT_NE(tool_request, nullptr);

    auto* move_mouse_request =
        static_cast<actor::MoveMouseToolRequest*>(tool_request.get());
    EXPECT_EQ(move_mouse_request->GetTabHandle(), test_tab_handle_);

    // Verify ToMojoToolAction conversion and check mojom properties
    auto* page_request =
        static_cast<actor::PageToolRequest*>(tool_request.get());
    auto mojo_action = page_request->ToMojoToolAction();
    ASSERT_TRUE(mojo_action);

    // Verify mojom action properties
    EXPECT_TRUE(mojo_action->is_mouse_move());
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<MockContentAgentTaskProvider> mock_task_provider_;
  std::unique_ptr<MoveMouseTool> move_mouse_tool_;
  tabs::TabHandle test_tab_handle_;
  actor::TaskId test_task_id_;
};

TEST_F(MoveMouseToolTest, ValidInputWithContentNode) {
  // Use standard content node target from target_test_util
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  std::string input_json = CreateValidMoveMouseJson(target_dict);
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      }));

  move_mouse_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify move mouse action properties
  VerifyMoveMouseAction(captured_actions);

  // Verify target separately using target_test_util
  const auto& target = captured_actions.actions(0).move_mouse().target();
  target_test_util::VerifyContentNodeTarget(target, 42, "doc123");
}

TEST_F(MoveMouseToolTest, ValidInputWithCoordinates) {
  // Use standard coordinate target from target_test_util
  auto target_dict = target_test_util::GetCoordinateTargetDict();
  std::string input_json = CreateValidMoveMouseJson(target_dict);
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      }));

  move_mouse_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify move mouse action properties
  VerifyMoveMouseAction(captured_actions);

  // Verify target separately using target_test_util
  const auto& target = captured_actions.actions(0).move_mouse().target();
  target_test_util::VerifyCoordinateTarget(target, 100, 200);
}

TEST_F(MoveMouseToolTest, ValidInputCustomContentNode) {
  // Use custom content node values
  auto target_dict =
      target_test_util::GetContentNodeTargetDict(777, "special_doc_id");
  std::string input_json = CreateValidMoveMouseJson(target_dict);
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

  move_mouse_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify move mouse action properties
  VerifyMoveMouseAction(captured_actions);

  // Verify target separately
  const auto& target = captured_actions.actions(0).move_mouse().target();
  target_test_util::VerifyContentNodeTarget(target, 777, "special_doc_id");
}

TEST_F(MoveMouseToolTest, ValidInputCustomCoordinates) {
  // Use custom coordinates for the target
  auto target_dict = target_test_util::GetCoordinateTargetDict(250.7, 350.3);
  std::string input_json = CreateValidMoveMouseJson(target_dict);
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

  move_mouse_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify move mouse action properties
  VerifyMoveMouseAction(captured_actions);

  // Verify target separately
  const auto& target = captured_actions.actions(0).move_mouse().target();
  target_test_util::VerifyCoordinateTarget(target, 250, 350);
}

TEST_F(MoveMouseToolTest, InvalidJson) {
  RunWithExpectedError("{ invalid json }",
                       "Failed to parse input JSON. Please try again.");
}

TEST_F(MoveMouseToolTest, MissingTargetObject) {
  std::string input_json = R"({})";

  RunWithExpectedError(input_json,
                       "Either content_node_id with document_identifier OR x,y "
                       "coordinates must be provided.");
}

// We only need minimal target validation tests since target_util_unittest.cc
// fully tests target validation already
TEST_F(MoveMouseToolTest, InvalidTargetValidation) {
  // Verify the tool properly handles invalid targets
  // and returns appropriate error messages from target_util
  RunWithExpectedError(CreateInvalidTargetJson("{}"),
                       "Target must contain one of either 'x' and 'y' or "
                       "'document_identifier' and optional 'content_node_id'");
}

TEST_F(MoveMouseToolTest, MissingDocumentIdentifierWithContentNode) {
  base::Value::Dict dict;

  base::Value::Dict target_dict;
  target_dict.Set("content_node_id", 42);
  // Note: Missing document_identifier intentionally
  dict.Set("target", target_dict.Clone());

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(input_json,
                       "Invalid identifiers: 'document_identifier' is required "
                       "when specifying 'content_node_id'");
}

TEST_F(MoveMouseToolTest, ToolMetadata) {
  EXPECT_EQ(move_mouse_tool_->Name(), "move_mouse");
  EXPECT_FALSE(std::string(move_mouse_tool_->Description()).empty());

  auto properties = move_mouse_tool_->InputProperties();
  EXPECT_TRUE(properties.has_value());

  auto required = move_mouse_tool_->RequiredProperties();
  EXPECT_TRUE(required.has_value());
  EXPECT_EQ(required->size(), 1u);  // target
}

}  // namespace ai_chat
