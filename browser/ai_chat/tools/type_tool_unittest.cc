// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/type_tool.h"

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
#include "chrome/browser/actor/tools/page_tool_request.h"
#include "chrome/browser/actor/tools/type_tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class TypeToolTest : public testing::Test {
 public:
  void SetUp() override {
    mock_task_provider_ = std::make_unique<MockContentAgentTaskProvider>();
    type_tool_ = std::make_unique<TypeTool>(mock_task_provider_.get());

    test_tab_handle_ = tabs::TabHandle(123);
    test_task_id_ = actor::TaskId(456);

    mock_task_provider_->SetTaskId(test_task_id_);

    ON_CALL(*mock_task_provider_, GetOrCreateTabHandleForTask)
        .WillByDefault(base::test::RunOnceCallback<0>(test_tab_handle_));
  }

 protected:
  // Creates a valid type JSON with the given target and type properties
  std::string CreateValidTypeJson(const base::Value::Dict& target_dict,
                                  const std::string& text = "test text",
                                  const std::string& mode = "replace",
                                  bool follow_by_enter = false) {
    base::Value::Dict dict;
    dict.Set("text", text);
    dict.Set("mode", mode);
    dict.Set("follow_by_enter", follow_by_enter);
    dict.Set("target", target_dict.Clone());

    std::string json;
    base::JSONWriter::Write(dict, &json);
    return json;
  }

  std::string CreateInvalidTargetJson(const std::string& target_content) {
    return R"({
      "text": "test text",
      "mode": "replace",
      "follow_by_enter": false,
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
    type_tool_->UseTool(input_json, future.GetCallback());

    auto result = future.Take();
    EXPECT_EQ(result.size(), 1u);
    ASSERT_TRUE(result[0]->is_text_content_block());
    EXPECT_EQ(result[0]->get_text_content_block()->text, expected_error);
  }

  // Verify type action properties and conversions
  void VerifyTypeAction(
      const optimization_guide::proto::Actions& actions,
      const std::string& expected_text,
      bool expected_follow_by_enter,
      optimization_guide::proto::TypeAction::TypeMode expected_mode) {
    // Verify proto action
    EXPECT_EQ(actions.task_id(), test_task_id_.value());
    EXPECT_EQ(actions.actions_size(), 1);

    const auto& action = actions.actions(0);
    EXPECT_TRUE(action.has_type());

    const auto& type_action = action.type();
    EXPECT_EQ(type_action.tab_id(), test_tab_handle_.raw_value());
    EXPECT_EQ(type_action.text(), expected_text);
    EXPECT_EQ(type_action.follow_by_enter(), expected_follow_by_enter);
    EXPECT_EQ(type_action.mode(), expected_mode);

    // Target verification should be handled by the target_test_util methods
    EXPECT_TRUE(type_action.has_target());

    // Verify CreateToolRequest works and produces correct TypeToolRequest
    auto tool_request = actor::CreateToolRequest(action, nullptr);
    ASSERT_NE(tool_request, nullptr);

    auto* type_request =
        static_cast<actor::TypeToolRequest*>(tool_request.get());
    EXPECT_EQ(type_request->GetTabHandle(), test_tab_handle_);
    EXPECT_EQ(type_request->text, expected_text);
    EXPECT_EQ(type_request->follow_by_enter, expected_follow_by_enter);

    // Convert mode enum values
    actor::TypeToolRequest::Mode expected_actor_mode;
    switch (expected_mode) {
      case optimization_guide::proto::TypeAction::DELETE_EXISTING:
        expected_actor_mode = actor::TypeToolRequest::Mode::kReplace;
        break;
      case optimization_guide::proto::TypeAction::PREPEND:
        expected_actor_mode = actor::TypeToolRequest::Mode::kPrepend;
        break;
      case optimization_guide::proto::TypeAction::APPEND:
        expected_actor_mode = actor::TypeToolRequest::Mode::kAppend;
        break;
      default:
        expected_actor_mode = actor::TypeToolRequest::Mode::kReplace;
        break;
    }
    EXPECT_EQ(type_request->mode, expected_actor_mode);

    // Verify ToMojoToolAction conversion and check mojom properties
    auto* page_request =
        static_cast<actor::PageToolRequest*>(tool_request.get());
    auto mojo_action = page_request->ToMojoToolAction();
    ASSERT_TRUE(mojo_action);

    // Verify mojom action properties
    EXPECT_TRUE(mojo_action->is_type());
    const auto& mojom_type = mojo_action->get_type();
    EXPECT_EQ(mojom_type->text, expected_text);
    EXPECT_EQ(mojom_type->follow_by_enter, expected_follow_by_enter);

    // Verify mode conversion
    actor::mojom::TypeAction::Mode expected_mojom_mode;
    switch (expected_actor_mode) {
      case actor::TypeToolRequest::Mode::kReplace:
        expected_mojom_mode = actor::mojom::TypeAction::Mode::kDeleteExisting;
        break;
      case actor::TypeToolRequest::Mode::kPrepend:
        expected_mojom_mode = actor::mojom::TypeAction::Mode::kPrepend;
        break;
      case actor::TypeToolRequest::Mode::kAppend:
        expected_mojom_mode = actor::mojom::TypeAction::Mode::kAppend;
        break;
    }
    EXPECT_EQ(mojom_type->mode, expected_mojom_mode);
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<MockContentAgentTaskProvider> mock_task_provider_;
  std::unique_ptr<TypeTool> type_tool_;
  tabs::TabHandle test_tab_handle_;
  actor::TaskId test_task_id_;
};

TEST_F(TypeToolTest, ValidInputWithContentNode) {
  // Use standard content node target from target_test_util
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  std::string input_json =
      CreateValidTypeJson(target_dict, "Hello World", "replace", true);
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      }));

  type_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify type action properties
  VerifyTypeAction(captured_actions, "Hello World", true,
                   optimization_guide::proto::TypeAction::DELETE_EXISTING);

  // Verify target separately using target_test_util
  const auto& target = captured_actions.actions(0).type().target();
  target_test_util::VerifyContentNodeTarget(target, 42, "doc123");
}

TEST_F(TypeToolTest, ValidInputWithCoordinates) {
  // Use standard coordinate target from target_test_util
  auto target_dict = target_test_util::GetCoordinateTargetDict();
  std::string input_json =
      CreateValidTypeJson(target_dict, "Append text", "append", false);
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      }));

  type_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify type action properties
  VerifyTypeAction(captured_actions, "Append text", false,
                   optimization_guide::proto::TypeAction::APPEND);

  // Verify target separately using target_test_util
  const auto& target = captured_actions.actions(0).type().target();
  target_test_util::VerifyCoordinateTarget(target, 100, 200);
}

TEST_F(TypeToolTest, ValidInputPrependMode) {
  // Use custom content node target with specific values
  auto target_dict = target_test_util::GetContentNodeTargetDict(99, "mydoc");
  std::string input_json =
      CreateValidTypeJson(target_dict, "Prepend: ", "prepend", false);
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

  type_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify type properties
  VerifyTypeAction(captured_actions, "Prepend: ", false,
                   optimization_guide::proto::TypeAction::PREPEND);

  // Verify target separately
  const auto& target = captured_actions.actions(0).type().target();
  target_test_util::VerifyContentNodeTarget(target, 99, "mydoc");
}

TEST_F(TypeToolTest, ValidInputCustomCoordinates) {
  // Use custom coordinates for the target
  auto target_dict = target_test_util::GetCoordinateTargetDict(250.7, 350.3);
  std::string input_json =
      CreateValidTypeJson(target_dict, "Custom coordinates", "replace", true);
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

  type_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify type properties
  VerifyTypeAction(captured_actions, "Custom coordinates", true,
                   optimization_guide::proto::TypeAction::DELETE_EXISTING);

  // Verify target separately
  const auto& target = captured_actions.actions(0).type().target();
  target_test_util::VerifyCoordinateTarget(target, 250, 350);
}

TEST_F(TypeToolTest, InvalidJson) {
  RunWithExpectedError("{ invalid json }",
                       "Failed to parse input JSON. Please try again.");
}

TEST_F(TypeToolTest, MissingText) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("mode", "replace");
  dict.Set("follow_by_enter", false);
  // Note: No text intentionally

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(input_json, "Missing required field: text");
}

TEST_F(TypeToolTest, MissingMode) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("text", "test text");
  dict.Set("follow_by_enter", false);
  // Note: No mode intentionally

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(
      input_json,
      "Invalid or missing mode. Must be 'replace', 'prepend', or 'append'.");
}

TEST_F(TypeToolTest, InvalidMode) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("text", "test text");
  dict.Set("mode", "invalid_mode");
  dict.Set("follow_by_enter", false);

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(
      input_json,
      "Invalid or missing mode. Must be 'replace', 'prepend', or 'append'.");
}

TEST_F(TypeToolTest, MissingFollowByEnter) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("text", "test text");
  dict.Set("mode", "replace");
  // Note: No follow_by_enter intentionally

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(input_json, "Missing required field: follow_by_enter");
}

TEST_F(TypeToolTest, MissingTargetObject) {
  std::string input_json = R"({
    "text": "test text",
    "mode": "replace",
    "follow_by_enter": false
  })";

  RunWithExpectedError(input_json, "Missing 'target' object");
}

// We only need minimal target validation tests since target_util_unittest.cc
// fully tests target validation already
TEST_F(TypeToolTest, InvalidTargetValidation) {
  // Verify the tool properly handles invalid targets
  // and returns appropriate error messages from target_util
  RunWithExpectedError(CreateInvalidTargetJson("{}"),
                       "Target must contain one of either 'x' and 'y' or "
                       "'document_identifier' and optional 'content_node_id'");
}

TEST_F(TypeToolTest, MissingDocumentIdentifierWithContentNode) {
  base::Value::Dict dict;
  dict.Set("text", "test text");
  dict.Set("mode", "replace");
  dict.Set("follow_by_enter", false);

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

TEST_F(TypeToolTest, ToolMetadata) {
  EXPECT_EQ(type_tool_->Name(), "type_text");
  EXPECT_FALSE(std::string(type_tool_->Description()).empty());

  auto properties = type_tool_->InputProperties();
  EXPECT_TRUE(properties.has_value());

  auto required = type_tool_->RequiredProperties();
  EXPECT_TRUE(required.has_value());
  EXPECT_EQ(required->size(), 4u);  // target, text, follow_by_enter, mode
}

}  // namespace ai_chat
