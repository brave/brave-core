// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/scroll_tool.h"

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
#include "chrome/browser/actor/tools/scroll_tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class ScrollToolTest : public testing::Test {
 public:
  void SetUp() override {
    mock_task_provider_ = std::make_unique<MockContentAgentTaskProvider>();
    scroll_tool_ =
        std::make_unique<ScrollTool>(mock_task_provider_.get(),
                                     nullptr);  // Actor service not used

    test_tab_handle_ = tabs::TabHandle(123);
    test_task_id_ = actor::TaskId(456);

    mock_task_provider_->SetTaskId(test_task_id_);

    ON_CALL(*mock_task_provider_, GetOrCreateTabHandleForTask)
        .WillByDefault(base::test::RunOnceCallback<0>(test_tab_handle_));
  }

 protected:
  // Creates a valid scroll JSON with the given target and scroll properties
  std::string CreateValidScrollJson(const base::Value::Dict& target_dict,
                                    const std::string& direction = "down",
                                    double distance = 100.0) {
    base::Value::Dict dict;
    dict.Set("direction", direction);
    dict.Set("distance", distance);
    dict.Set("target", target_dict.Clone());

    std::string json;
    base::JSONWriter::Write(dict, &json);
    return json;
  }

  std::string CreateInvalidTargetJson(const std::string& target_content) {
    return R"({
      "direction": "down",
      "distance": 100.0,
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
    scroll_tool_->UseTool(input_json, future.GetCallback());

    auto result = future.Take();
    EXPECT_EQ(result.size(), 1u);
    ASSERT_TRUE(result[0]->is_text_content_block());
    EXPECT_EQ(result[0]->get_text_content_block()->text, expected_error);
  }

  // Verify scroll action properties and conversions
  void VerifyScrollAction(
      const optimization_guide::proto::Actions& actions,
      optimization_guide::proto::ScrollAction::ScrollDirection
          expected_direction,
      float expected_distance) {
    // Verify proto action
    EXPECT_EQ(actions.task_id(), test_task_id_.value());
    EXPECT_EQ(actions.actions_size(), 1);

    const auto& action = actions.actions(0);
    EXPECT_TRUE(action.has_scroll());

    const auto& scroll_action = action.scroll();
    EXPECT_EQ(scroll_action.tab_id(), test_tab_handle_.raw_value());
    EXPECT_EQ(scroll_action.direction(), expected_direction);
    EXPECT_FLOAT_EQ(scroll_action.distance(), expected_distance);

    // Target verification should be handled by the target_test_util methods
    EXPECT_TRUE(scroll_action.has_target());

    // Verify CreateToolRequest works and produces correct ScrollToolRequest
    auto tool_request = actor::CreateToolRequest(action, nullptr);
    ASSERT_NE(tool_request, nullptr);

    auto* scroll_request =
        static_cast<actor::ScrollToolRequest*>(tool_request.get());
    EXPECT_EQ(scroll_request->GetTabHandle(), test_tab_handle_);

    // Convert direction enum values
    actor::ScrollToolRequest::Direction expected_actor_direction;
    switch (expected_direction) {
      case optimization_guide::proto::ScrollAction::LEFT:
        expected_actor_direction = actor::ScrollToolRequest::Direction::kLeft;
        break;
      case optimization_guide::proto::ScrollAction::RIGHT:
        expected_actor_direction = actor::ScrollToolRequest::Direction::kRight;
        break;
      case optimization_guide::proto::ScrollAction::UP:
        expected_actor_direction = actor::ScrollToolRequest::Direction::kUp;
        break;
      case optimization_guide::proto::ScrollAction::DOWN:
        expected_actor_direction = actor::ScrollToolRequest::Direction::kDown;
        break;
      default:
        expected_actor_direction = actor::ScrollToolRequest::Direction::kDown;
        break;
    }

    // Verify ToMojoToolAction conversion and check mojom properties
    auto* page_request =
        static_cast<actor::PageToolRequest*>(tool_request.get());
    auto mojo_action = page_request->ToMojoToolAction();
    ASSERT_TRUE(mojo_action);

    // Verify mojom action properties
    EXPECT_TRUE(mojo_action->is_scroll());
    const auto& mojom_scroll = mojo_action->get_scroll();
    EXPECT_FLOAT_EQ(mojom_scroll->distance, expected_distance);

    // Verify direction conversion
    actor::mojom::ScrollAction::ScrollDirection expected_mojom_direction;
    switch (expected_actor_direction) {
      case actor::ScrollToolRequest::Direction::kLeft:
        expected_mojom_direction =
            actor::mojom::ScrollAction::ScrollDirection::kLeft;
        break;
      case actor::ScrollToolRequest::Direction::kRight:
        expected_mojom_direction =
            actor::mojom::ScrollAction::ScrollDirection::kRight;
        break;
      case actor::ScrollToolRequest::Direction::kUp:
        expected_mojom_direction =
            actor::mojom::ScrollAction::ScrollDirection::kUp;
        break;
      case actor::ScrollToolRequest::Direction::kDown:
        expected_mojom_direction =
            actor::mojom::ScrollAction::ScrollDirection::kDown;
        break;
    }
    EXPECT_EQ(mojom_scroll->direction, expected_mojom_direction);
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<MockContentAgentTaskProvider> mock_task_provider_;
  std::unique_ptr<ScrollTool> scroll_tool_;
  tabs::TabHandle test_tab_handle_;
  actor::TaskId test_task_id_;
};

TEST_F(ScrollToolTest, ValidInputWithDocumentTargetDown) {
  // Use standard content node target from target_test_util
  auto target_dict = target_test_util::GetDocumentTargetDict();
  std::string input_json = CreateValidScrollJson(target_dict, "down", 150.0);
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      }));

  scroll_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify scroll action properties
  VerifyScrollAction(captured_actions,
                     optimization_guide::proto::ScrollAction::DOWN, 150.0f);

  // Verify target separately using target_test_util
  const auto& target = captured_actions.actions(0).scroll().target();
  target_test_util::VerifyDocumentTarget(target, "doc123");
}

TEST_F(ScrollToolTest, ValidInputWithContentNodeDown) {
  // Use standard content node target from target_test_util
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  std::string input_json = CreateValidScrollJson(target_dict, "down", 150.0);
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      }));

  scroll_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify scroll action properties
  VerifyScrollAction(captured_actions,
                     optimization_guide::proto::ScrollAction::DOWN, 150.0f);

  // Verify target separately using target_test_util
  const auto& target = captured_actions.actions(0).scroll().target();
  target_test_util::VerifyContentNodeTarget(target, 42, "doc123");
}

TEST_F(ScrollToolTest, ValidInputWithCoordinatesUp) {
  // Use standard coordinate target from target_test_util
  auto target_dict = target_test_util::GetCoordinateTargetDict();
  std::string input_json = CreateValidScrollJson(target_dict, "up", 250.5);
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      }));

  scroll_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify scroll action properties
  VerifyScrollAction(captured_actions,
                     optimization_guide::proto::ScrollAction::UP, 250.5f);

  // Verify target separately using target_test_util
  const auto& target = captured_actions.actions(0).scroll().target();
  target_test_util::VerifyCoordinateTarget(target, 100, 200);
}

TEST_F(ScrollToolTest, ValidInputLeftDirection) {
  // Use custom content node target with specific values
  auto target_dict = target_test_util::GetContentNodeTargetDict(99, "mydoc");
  std::string input_json = CreateValidScrollJson(target_dict, "left", 50.0);
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

  scroll_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify scroll properties
  VerifyScrollAction(captured_actions,
                     optimization_guide::proto::ScrollAction::LEFT, 50.0f);

  // Verify target separately
  const auto& target = captured_actions.actions(0).scroll().target();
  target_test_util::VerifyContentNodeTarget(target, 99, "mydoc");
}

TEST_F(ScrollToolTest, ValidInputRightDirection) {
  // Use custom coordinates for the target
  auto target_dict = target_test_util::GetCoordinateTargetDict(250.7, 350.3);
  std::string input_json = CreateValidScrollJson(target_dict, "right", 75.25);
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

  scroll_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify scroll properties
  VerifyScrollAction(captured_actions,
                     optimization_guide::proto::ScrollAction::RIGHT, 75.25f);

  // Verify target separately
  const auto& target = captured_actions.actions(0).scroll().target();
  target_test_util::VerifyCoordinateTarget(target, 250, 350);
}

TEST_F(ScrollToolTest, InvalidJson) {
  RunWithExpectedError("{ invalid json }",
                       "Failed to parse input JSON. Please try again.");
}

TEST_F(ScrollToolTest, MissingDirection) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("distance", 100.0);
  // Note: No direction intentionally

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(input_json,
                       "Invalid or missing direction. Must be 'left', 'right', "
                       "'up', or 'down'.");
}

TEST_F(ScrollToolTest, InvalidDirection) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("direction", "diagonal");  // Invalid direction
  dict.Set("distance", 100.0);

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(input_json,
                       "Invalid or missing direction. Must be 'left', 'right', "
                       "'up', or 'down'.");
}

TEST_F(ScrollToolTest, MissingDistance) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("direction", "down");
  // Note: No distance intentionally

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(
      input_json, "Invalid or missing distance. Must be a positive number.");
}

TEST_F(ScrollToolTest, NegativeDistance) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("direction", "down");
  dict.Set("distance", -50.0);  // Negative distance

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(
      input_json, "Invalid or missing distance. Must be a positive number.");
}

TEST_F(ScrollToolTest, ZeroDistance) {
  auto target_dict = target_test_util::GetContentNodeTargetDict();
  base::Value::Dict dict;
  dict.Set("target", target_dict.Clone());
  dict.Set("direction", "down");
  dict.Set("distance", 0.0);  // Zero distance

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(
      input_json, "Invalid or missing distance. Must be a positive number.");
}

TEST_F(ScrollToolTest, MissingDocumentIdentifier) {
  base::Value::Dict dict;
  dict.Set("direction", "down");
  dict.Set("distance", 100.0);

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

TEST_F(ScrollToolTest, MissingTargetObject) {
  base::Value::Dict dict;
  dict.Set("direction", "down");
  dict.Set("distance", 100.0);
  // Note: No target intentionally

  std::string input_json;
  base::JSONWriter::Write(dict, &input_json);

  RunWithExpectedError(input_json, "Missing 'target' object");
}

// We only need minimal target validation tests since target_util_unittest.cc
// fully tests target validation already
TEST_F(ScrollToolTest, InvalidTargetValidation) {
  // Verify the tool properly handles invalid targets
  // and returns appropriate error messages from target_util
  RunWithExpectedError(CreateInvalidTargetJson("{}"),
                       "Target must contain one of either 'x' and 'y' or "
                       "'document_identifier' and optional 'content_node_id'");
}

TEST_F(ScrollToolTest, ToolMetadata) {
  EXPECT_EQ(scroll_tool_->Name(), "scroll_element");
  EXPECT_FALSE(std::string(scroll_tool_->Description()).empty());

  auto properties = scroll_tool_->InputProperties();
  EXPECT_TRUE(properties.has_value());

  auto required = scroll_tool_->RequiredProperties();
  EXPECT_TRUE(required.has_value());
  EXPECT_EQ(required->size(), 3u);  // target, direction, distance
}

}  // namespace ai_chat
