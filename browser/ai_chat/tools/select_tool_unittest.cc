// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/select_tool.h"

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
#include "chrome/browser/actor/tools/select_tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class SelectToolTest : public testing::Test {
 public:
  void SetUp() override {
    mock_task_provider_ = std::make_unique<MockContentAgentTaskProvider>();
    select_tool_ = std::make_unique<SelectTool>(mock_task_provider_.get());

    test_tab_handle_ = tabs::TabHandle(123);
    test_task_id_ = actor::TaskId(456);

    mock_task_provider_->SetTaskId(test_task_id_);

    ON_CALL(*mock_task_provider_, GetOrCreateTabHandleForTask)
        .WillByDefault(base::test::RunOnceCallback<0>(test_tab_handle_));
  }

 protected:
  // Creates a valid select JSON with the given target and value
  std::string CreateValidSelectJson(const base::Value::Dict& target_dict,
                                    const std::string& value = "option1") {
    base::Value::Dict dict;
    dict.Set("value", value);
    dict.Set("target", target_dict.Clone());

    std::string json;
    base::JSONWriter::Write(dict, &json);
    return json;
  }

  std::string CreateInvalidTargetJson(const std::string& target_content) {
    return R"({
      "value": "option1",
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
    select_tool_->UseTool(input_json, future.GetCallback());

    auto result = future.Take();
    EXPECT_EQ(result.size(), 1u);
    ASSERT_TRUE(result[0]->is_text_content_block());
    EXPECT_EQ(result[0]->get_text_content_block()->text, expected_error);
  }

  // Verify select action properties and conversions
  void VerifySelectAction(const optimization_guide::proto::Actions& actions,
                          const std::string& expected_value) {
    // Verify proto action
    EXPECT_EQ(actions.task_id(), test_task_id_.value());
    EXPECT_EQ(actions.actions_size(), 1);

    const auto& action = actions.actions(0);
    EXPECT_TRUE(action.has_select());

    const auto& select_action = action.select();
    EXPECT_EQ(select_action.tab_id(), test_tab_handle_.raw_value());
    EXPECT_EQ(select_action.value(), expected_value);

    // Target verification should be handled by the target_test_util methods
    EXPECT_TRUE(select_action.has_target());

    // Verify CreateToolRequest works and produces correct SelectToolRequest
    auto tool_request = actor::CreateToolRequest(action, nullptr);
    ASSERT_NE(tool_request, nullptr);

    auto* select_request =
        static_cast<actor::SelectToolRequest*>(tool_request.get());
    EXPECT_EQ(select_request->GetTabHandle(), test_tab_handle_);

    // Verify ToMojoToolAction conversion and check mojom properties
    auto* page_request =
        static_cast<actor::PageToolRequest*>(tool_request.get());
    auto mojo_action = page_request->ToMojoToolAction();
    ASSERT_TRUE(mojo_action);

    // Verify mojom action properties
    EXPECT_TRUE(mojo_action->is_select());
    const auto& mojom_select = mojo_action->get_select();
    EXPECT_EQ(mojom_select->value, expected_value);
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<MockContentAgentTaskProvider> mock_task_provider_;
  std::unique_ptr<SelectTool> select_tool_;
  tabs::TabHandle test_tab_handle_;
  actor::TaskId test_task_id_;
};

TEST_F(SelectToolTest, ValidInputWithContentNode) {
  // Use standard content node target from target_test_util
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  std::string input_json = CreateValidSelectJson(target_dict, "option1");
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      }));

  select_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify select action properties
  VerifySelectAction(captured_actions, "option1");

  // Verify target separately using target_test_util
  const auto& target = captured_actions.actions(0).select().target();
  target_test_util::VerifyContentNodeTarget(target, 42, "doc123");
}

TEST_F(SelectToolTest, ValidInputWithCoordinates) {
  // Use standard coordinate target from target_test_util
  auto target_dict = target_test_util::GetCoordinateTargetDict();
  std::string input_json = CreateValidSelectJson(target_dict, "value2");
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      }));

  select_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify select action properties
  VerifySelectAction(captured_actions, "value2");

  // Verify target separately using target_test_util
  const auto& target = captured_actions.actions(0).select().target();
  target_test_util::VerifyCoordinateTarget(target, 100, 200);
}

TEST_F(SelectToolTest, ValidInputComplexValue) {
  // Use custom content node target with specific values
  auto target_dict = target_test_util::GetContentNodeTargetDict(99, "mydoc");
  std::string input_json =
      CreateValidSelectJson(target_dict, "complex-option-value-123");
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

  select_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify select properties
  VerifySelectAction(captured_actions, "complex-option-value-123");

  // Verify target separately
  const auto& target = captured_actions.actions(0).select().target();
  target_test_util::VerifyContentNodeTarget(target, 99, "mydoc");
}

TEST_F(SelectToolTest, ValidInputCustomCoordinates) {
  // Use custom coordinates for the target
  auto target_dict = target_test_util::GetCoordinateTargetDict(250.7, 350.3);
  std::string input_json = CreateValidSelectJson(target_dict, "custom-value");
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

  select_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify select properties
  VerifySelectAction(captured_actions, "custom-value");

  // Verify target separately
  const auto& target = captured_actions.actions(0).select().target();
  target_test_util::VerifyCoordinateTarget(target, 250, 350);
}

TEST_F(SelectToolTest, InvalidJson) {
  RunWithExpectedError("{ invalid json }",
                       "Failed to parse input JSON. Please try again.");
}

TEST_F(SelectToolTest, MissingValue) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  // Note: No value intentionally

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(input_json,
                       "Missing required parameter 'value' - the value "
                       "attribute of the option to select.");
}

TEST_F(SelectToolTest, MissingTargetObject) {
  std::string input_json = R"({
    "value": "option1"
  })";

  RunWithExpectedError(input_json, "Missing 'target' object");
}

// We only need minimal target validation tests since target_util_unittest.cc
// fully tests target validation already
TEST_F(SelectToolTest, InvalidTargetValidation) {
  // Verify the tool properly handles invalid targets
  // and returns appropriate error messages from target_util
  RunWithExpectedError(CreateInvalidTargetJson("{}"),
                       "Target must contain one of either 'x' and 'y' or "
                       "'document_identifier' and optional 'content_node_id'");
}

TEST_F(SelectToolTest, MissingDocumentIdentifierWithContentNode) {
  base::Value::Dict dict;
  dict.Set("value", "option1");

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

TEST_F(SelectToolTest, InvalidValueType) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("value", 123);  // Invalid type - should be string

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(input_json,
                       "Missing required parameter 'value' - the value "
                       "attribute of the option to select.");
}

TEST_F(SelectToolTest, ToolMetadata) {
  EXPECT_EQ(select_tool_->Name(), "select_dropdown");
  EXPECT_FALSE(std::string(select_tool_->Description()).empty());

  auto properties = select_tool_->InputProperties();
  EXPECT_TRUE(properties.has_value());

  auto required = select_tool_->RequiredProperties();
  EXPECT_TRUE(required.has_value());
  EXPECT_EQ(required->size(), 2u);  // target, value
}

}  // namespace ai_chat
