// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/drag_and_release_tool.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "base/run_loop.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/tools/mock_content_agent_task_provider.h"
#include "brave/browser/ai_chat/tools/target_test_util.h"
#include "chrome/browser/actor/browser_action_util.h"
#include "chrome/browser/actor/task_id.h"
#include "chrome/browser/actor/tools/drag_and_release_tool_request.h"
#include "chrome/browser/actor/tools/page_tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class DragAndReleaseToolTest : public testing::Test {
 public:
  void SetUp() override {
    mock_task_provider_ = std::make_unique<MockContentAgentTaskProvider>();
    drag_and_release_tool_ = std::make_unique<DragAndReleaseTool>(
        mock_task_provider_.get(), nullptr);  // Actor service not used

    test_tab_handle_ = tabs::TabHandle(123);
    test_task_id_ = actor::TaskId(456);

    mock_task_provider_->SetTaskId(test_task_id_);

    ON_CALL(*mock_task_provider_, GetOrCreateTabHandleForTask)
        .WillByDefault(base::test::RunOnceCallback<0>(test_tab_handle_));
  }

 protected:
  // Creates a valid drag and release JSON with the given source and target
  std::string CreateValidDragAndReleaseJson(
      const base::Value::Dict& from_target,
      const base::Value::Dict& to_target) {
    base::Value::Dict dict;
    dict.Set("from", from_target.Clone());
    dict.Set("to", to_target.Clone());

    std::string json;
    base::JSONWriter::Write(dict, &json);
    return json;
  }

  void RunWithExpectedError(base::Location location,
                            const std::string& input_json,
                            const std::string& expected_error) {
    SCOPED_TRACE(testing::Message() << location.ToString());
    // For error cases, the tool should not call the interesting task provider
    // methods
    EXPECT_CALL(*mock_task_provider_, GetOrCreateTabHandleForTask).Times(0);
    EXPECT_CALL(*mock_task_provider_, ExecuteActions).Times(0);

    base::test::TestFuture<std::vector<mojom::ContentBlockPtr>> future;
    drag_and_release_tool_->UseTool(input_json, future.GetCallback());

    auto result = future.Take();
    EXPECT_EQ(result.size(), 1u);
    ASSERT_TRUE(result[0]->is_text_content_block());
    EXPECT_EQ(result[0]->get_text_content_block()->text, expected_error);
  }

  // Verify drag and release action and tool request creation
  void VerifyDragAndReleaseAction(
      const optimization_guide::proto::Actions& actions) {
    // Verify proto action
    EXPECT_EQ(actions.task_id(), test_task_id_.value());
    EXPECT_EQ(actions.actions_size(), 1);

    const auto& action = actions.actions(0);
    EXPECT_TRUE(action.has_drag_and_release());

    const auto& drag_action = action.drag_and_release();
    EXPECT_EQ(drag_action.tab_id(), test_tab_handle_.raw_value());

    // Both targets should be present
    EXPECT_TRUE(drag_action.has_from_target());
    EXPECT_TRUE(drag_action.has_to_target());

    // Verify tool request creation works correctly
    auto tool_request = actor::CreateToolRequest(action, nullptr);
    ASSERT_NE(tool_request, nullptr);

    auto* drag_request =
        static_cast<actor::DragAndReleaseToolRequest*>(tool_request.get());
    EXPECT_EQ(drag_request->GetTabHandle(), test_tab_handle_);

    // Verify ToMojoToolAction conversion
    auto* page_request =
        static_cast<actor::PageToolRequest*>(tool_request.get());
    auto mojo_action = page_request->ToMojoToolAction();
    ASSERT_TRUE(mojo_action);

    // Verify mojom action properties
    EXPECT_TRUE(mojo_action->is_drag_and_release());
    const auto& mojom_drag = mojo_action->get_drag_and_release();
    EXPECT_TRUE(mojom_drag->to_target);
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<MockContentAgentTaskProvider> mock_task_provider_;
  std::unique_ptr<DragAndReleaseTool> drag_and_release_tool_;
  tabs::TabHandle test_tab_handle_;
  actor::TaskId test_task_id_;
};

// Test valid drag from content node to content node
TEST_F(DragAndReleaseToolTest, DragFromContentNodeToContentNode) {
  // Use standard content node targets from target_test_util
  auto from_target =
      target_test_util::GetContentNodeTargetDict(42, "source_doc");
  auto to_target = target_test_util::GetContentNodeTargetDict(99, "dest_doc");
  std::string input_json =
      CreateValidDragAndReleaseJson(from_target, to_target);
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      }));

  drag_and_release_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify drag action properties
  VerifyDragAndReleaseAction(captured_actions);

  // Verify from target separately
  const auto& from_target_proto =
      captured_actions.actions(0).drag_and_release().from_target();
  target_test_util::VerifyContentNodeTarget(from_target_proto, 42,
                                            "source_doc");

  // Verify to target separately
  const auto& to_target_proto =
      captured_actions.actions(0).drag_and_release().to_target();
  target_test_util::VerifyContentNodeTarget(to_target_proto, 99, "dest_doc");
}

// Test valid drag from content node to coordinates
TEST_F(DragAndReleaseToolTest, DragFromContentNodeToCoordinates) {
  auto from_target =
      target_test_util::GetContentNodeTargetDict(42, "source_doc");
  auto to_target = target_test_util::GetCoordinateTargetDict(250.7, 350.3);
  std::string input_json =
      CreateValidDragAndReleaseJson(from_target, to_target);
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      }));

  drag_and_release_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify drag action properties
  VerifyDragAndReleaseAction(captured_actions);

  // Verify from target separately
  const auto& from_target_proto =
      captured_actions.actions(0).drag_and_release().from_target();
  target_test_util::VerifyContentNodeTarget(from_target_proto, 42,
                                            "source_doc");

  // Verify to target separately
  const auto& to_target_proto =
      captured_actions.actions(0).drag_and_release().to_target();
  target_test_util::VerifyCoordinateTarget(to_target_proto, 250, 350);
}

// Test valid drag from coordinates to coordinates
TEST_F(DragAndReleaseToolTest, DragFromCoordinatesToCoordinates) {
  auto from_target = target_test_util::GetCoordinateTargetDict(100.5, 200.5);
  auto to_target = target_test_util::GetCoordinateTargetDict(250.7, 350.3);
  std::string input_json =
      CreateValidDragAndReleaseJson(from_target, to_target);
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      }));

  drag_and_release_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify drag action properties
  VerifyDragAndReleaseAction(captured_actions);

  // Verify from target separately
  const auto& from_target_proto =
      captured_actions.actions(0).drag_and_release().from_target();
  target_test_util::VerifyCoordinateTarget(from_target_proto, 100, 200);

  // Verify to target separately
  const auto& to_target_proto =
      captured_actions.actions(0).drag_and_release().to_target();
  target_test_util::VerifyCoordinateTarget(to_target_proto, 250, 350);
}

// Test invalid JSON format
TEST_F(DragAndReleaseToolTest, InvalidJson) {
  RunWithExpectedError(FROM_HERE, "invalid json",
                       "Failed to parse input JSON. Please try again.");
}

// Test missing from target
TEST_F(DragAndReleaseToolTest, MissingFromTarget) {
  auto to_target = target_test_util::GetContentNodeTargetDict();
  std::string input_json =
      absl::StrFormat(R"({"to": %s})", base::WriteJson(to_target).value());

  RunWithExpectedError(FROM_HERE, input_json, "Missing 'from' target object");
}

// Test missing to target
TEST_F(DragAndReleaseToolTest, MissingToTarget) {
  auto from_target = target_test_util::GetContentNodeTargetDict();
  std::string input_json =
      absl::StrFormat(R"({"from": %s})", base::WriteJson(from_target).value());

  RunWithExpectedError(FROM_HERE, input_json, "Missing 'to' target object");
}

// Test invalid from target
TEST_F(DragAndReleaseToolTest, InvalidFromTarget) {
  // Invalid from target (empty object)
  RunWithExpectedError(
      FROM_HERE, R"({ "from": {} })",
      "Invalid 'from' target: Target must contain one of either 'x' and 'y' or "
      "'document_identifier' and optional 'content_node_id'");
}

// Test invalid to target
TEST_F(DragAndReleaseToolTest, InvalidToTarget) {
  auto from_target = target_test_util::GetContentNodeTargetDict();
  std::string input_json = absl::StrFormat(
      R"({"from": %s, "to": {}})", base::WriteJson(from_target).value());

  RunWithExpectedError(
      FROM_HERE, input_json,
      "Invalid 'to' target: Target must contain one of either 'x' and 'y' or "
      "'document_identifier' and optional 'content_node_id'");
}

// Test tool metadata
TEST_F(DragAndReleaseToolTest, ToolMetadata) {
  EXPECT_EQ(drag_and_release_tool_->Name(), "drag_and_release");
  EXPECT_FALSE(std::string(drag_and_release_tool_->Description()).empty());

  auto properties = drag_and_release_tool_->InputProperties();
  EXPECT_TRUE(properties.has_value());

  auto required = drag_and_release_tool_->RequiredProperties();
  EXPECT_TRUE(required.has_value());
  EXPECT_EQ(required->size(), 2u);  // from, to
}

}  // namespace ai_chat
